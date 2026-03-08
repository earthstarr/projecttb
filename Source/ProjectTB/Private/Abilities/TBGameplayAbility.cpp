#include "Abilities/TBGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Attributes/TBAttributeSet.h"
#include "Battle/BattleManager.h"
#include "Battle/BattleCombatant.h"
#include "TBGameplayTags.h"
#include "Battle/BattleImpactActor.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

UTBGameplayAbility::UTBGameplayAbility()
{
	// 어빌리티 인스턴싱: 액터당 1개 (단일 플레이어 턴제에 적합)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UTBGameplayAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// DamageEffectClass 없음 → Blueprint Event ActivateAbility 실행
	if (!DamageEffectClass)
	{
		Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	bAbilityFinished = false;
	bImpactSpawned = false;
	OriginLocation = FVector::ZeroVector;

	AActor* Avatar = CurrentActorInfo->AvatarActor.Get();
	if (Avatar)
	{
		// 현재 회전값 저장 (BattleManager::ExecuteAction에서 이미 타겟 방향으로 회전됨)
		OriginRotation = Avatar->GetActorRotation();
	}

	// 참고: 캐릭터 회전은 BattleManager::ExecuteAction에서 카메라 전환 전에 수행됨

	// Melee: 0.5초 후 순간이동
	if (AnimationType == EAbilityAnimType::Melee)
	{
		if (Avatar)
		{
			Avatar->GetWorldTimerManager().SetTimer(PreMoveTimer, this, &UTBGameplayAbility::DoTeleportToTarget, 0.5f, false);
		}
		return;
	}

	// Ranged / Impact: 바로 몽타주 재생 (컷씬은 몽타주 후에)
	DoPlayMontage();
}

void UTBGameplayAbility::DoTeleportToTarget()
{
	AActor* Avatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar) { FinishAbility(); return; }

	ABattleManager* BM = GetBattleManager();
	if (!BM) { FinishAbility(); return; }

	OriginLocation = Avatar->GetActorLocation();

	// AllEnemies/AllAllies: 모든 타겟의 중심으로 이동
	if (TargetType == EAbilityTargetType::AllEnemies || TargetType == EAbilityTargetType::AllAllies)
	{
		TArray<ABattleCombatant*> Targets;
		if (TargetType == EAbilityTargetType::AllEnemies)
			Targets = BM->GetLivingEnemies();
		else
			Targets = BM->GetLivingPlayers();

		if (!Targets.IsEmpty())
		{
			// 모든 타겟의 중심 계산
			FVector CenterLocation = FVector::ZeroVector;
			for (ABattleCombatant* T : Targets)
				if (T) CenterLocation += T->GetActorLocation();
			CenterLocation /= Targets.Num();

			const FVector Dir = (OriginLocation - CenterLocation).GetSafeNormal();
			const FVector AttackPos = CenterLocation + Dir * 200.f;
			Avatar->SetActorLocation(FVector(AttackPos.X, AttackPos.Y, OriginLocation.Z));

			const FVector LookDir = (CenterLocation - AttackPos).GetSafeNormal();
			Avatar->SetActorRotation(LookDir.Rotation());
		}
	}
	// SingleEnemy/SingleAlly: 단일 타겟으로 이동
	else if (ABattleCombatant* Target = BM->GetPendingTarget())
	{
		const FVector Dir = (OriginLocation - Target->GetActorLocation()).GetSafeNormal();
		const FVector AttackPos = Target->GetActorLocation() + Dir * 200.f;
		Avatar->SetActorLocation(FVector(AttackPos.X, AttackPos.Y, OriginLocation.Z));

		const FVector LookDir = (Target->GetActorLocation() - AttackPos).GetSafeNormal();
		Avatar->SetActorRotation(LookDir.Rotation());
	}

	// 0.5초 후 몽타주 재생
	Avatar->GetWorldTimerManager().SetTimer(
		PreMontageTimer, this, &UTBGameplayAbility::DoPlayMontage, 0.5f, false);
}

void UTBGameplayAbility::DoPlayMontage()
{
	// 몽타주 카메라 활성화 시 카메라 블렌딩 완료 후 몽타주 재생
	if (bUseActionCamera && bUseMontageCamera)
	{
		AActor* Avatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr;
		ABattleManager* BM = GetBattleManager();

		if (Avatar && BM)
		{
			const FTransform AvatarTransform = Avatar->GetActorTransform();
			const FVector MontageWorldLocation = AvatarTransform.TransformPosition(MontageCameraLocalOffset);
			const FRotator MontageWorldRotation = AvatarTransform.TransformRotation(
				MontageCameraLocalRotation.Quaternion()).Rotator();

			BM->SetActionCameraWorldPosition(MontageWorldLocation, MontageWorldRotation, MontageCameraBlendInTime);

			// 카메라 블렌딩 완료 후 몽타주 재생
			Avatar->GetWorldTimerManager().SetTimer(
				MontageCameraBlendTimer, this, &UTBGameplayAbility::OnMontageCameraBlendFinished, MontageCameraBlendInTime, false);
			return;
		}
	}

	// 몽타주 카메라를 사용하지 않는 경우 바로 몽타주 재생
	PlayMontageInternal();
}

void UTBGameplayAbility::OnMontageCameraBlendFinished()
{
	PlayMontageInternal();
}

void UTBGameplayAbility::PlayMontageInternal()
{
	if (AbilityMontage)
	{
		UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, AbilityMontage);
		Task->OnCompleted.AddDynamic(this, &UTBGameplayAbility::OnMontageCompleted);
		Task->OnBlendOut.AddDynamic(this, &UTBGameplayAbility::OnMontageBlendOut);
		Task->OnInterrupted.AddDynamic(this, &UTBGameplayAbility::OnMontageCompleted);
		Task->OnCancelled.AddDynamic(this, &UTBGameplayAbility::OnMontageCompleted);
		Task->ReadyForActivation();
		return;
	}

	// 몽타주 없음 → Impact면 액터 스폰, 아니면 바로 종료
	if (AnimationType == EAbilityAnimType::Impact)
	{
		SpawnImpactActor();
	}
	else
	{
		FinishAbility();
	}
}

void UTBGameplayAbility::OnMontageBlendOut()
{
	if (AnimationType == EAbilityAnimType::Impact)
	{
		if (!bImpactSpawned)
		{
			// 컷씬 카메라가 설정되어 있으면 컷씬 먼저 재생
			if (bUseActionCamera && bUseCutsceneCamera)
			{
				StartCutsceneCamera();
			}
			else if (bUseActionCamera)
			{
				// 컷씬 없이 액션 카메라만 사용하는 경우 → 액션 카메라로 전환 후 스폰
				SwitchToActionCameraAndSpawnImpact();
			}
			else
			{
				SpawnImpactActor();
			}
		}
	}
	else
	{
		FinishAbility();
	}
}

void UTBGameplayAbility::OnMontageCompleted()
{
	// BlendOut이 없었을 경우 대비 (중단/취소 시)
	// Impact는 이미 SpawnImpactActor에서 처리되므로 스킵
	if (AnimationType != EAbilityAnimType::Impact)
		FinishAbility();
}

// ─── 데미지 적용 (AnimNotify_OnHit → BattleCombatant::OnHitNotify → 여기) ──────
void UTBGameplayAbility::ApplyDamage(int32 HitIndex)
{
	if (!IsActive()) return;
		
	if (!DamageEffectClass) return;
	
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsValid(Avatar)) return;

	ABattleManager* BM = GetBattleManager();
	if (!BM) return;

	const float Multiplier = HitMultipliers.IsValidIndex(HitIndex) ? HitMultipliers[HitIndex] : 1.0f;

	// TargetType에 따라 단일/전체 타겟 처리
	TArray<ABattleCombatant*> Targets;

	switch (TargetType)
	{
	case EAbilityTargetType::AllEnemies:
		Targets = BM->GetLivingEnemies();
		break;
	case EAbilityTargetType::AllAllies:
		Targets = BM->GetLivingPlayers();
		break;
	case EAbilityTargetType::Self:
		if (ABattleCombatant* Caster = Cast<ABattleCombatant>(CurrentActorInfo->AvatarActor.Get()))
			Targets.Add(Caster);
		break;
	case EAbilityTargetType::SingleEnemy:
	case EAbilityTargetType::SingleAlly:
	default:
		if (ABattleCombatant* SingleTarget = BM->GetPendingTarget())
			Targets.Add(SingleTarget);
		break;
	}

	// 모든 타겟에 GE + 상태이상 적용
	for (ABattleCombatant* Target : Targets)
	{
		if (!IsValid(Target) || Target->IsDead()) continue;

		UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
		if (!TargetASC) continue;

		// ─── 데미지 GE 적용 ─────────────────────────────────────────────────
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
			CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageEffectClass, 1.f);

		if (AbilityTypeTag.IsValid())
			SpecHandle.Data->CapturedSourceTags.GetSpecTags().AddTag(AbilityTypeTag);

		SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_AbilityMultiplier, Multiplier);
		TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

		// ─── 상태이상 부여 ───────────────────────────────────────────────────
		if (!OnHitStatusEffects.IsEmpty())
		{
			// 시전자 MagicAttack 스냅샷
			ABattleCombatant* Caster = Cast<ABattleCombatant>(CurrentActorInfo->AvatarActor.Get());
			const float CasterMagic = Caster ? Caster->GetMagicAttack() : 0.f;

			for (const FStatusEffectConfig& Config : OnHitStatusEffects)
			{
				if (!Config.StatusTag.IsValid() || Config.StacksToApply <= 0) continue;

				FStatusEffectInstance Instance;
				Instance.StatusTag       = Config.StatusTag;
				Instance.MagnitudePerStack = CasterMagic * Config.ScalingCoeff;
				Instance.RemainingTurns  = Config.StacksToApply;

				Target->ApplyStatusEffect(Instance);
			}
		}
	}
}

// ─── 어빌리티 종료 ────────────────────────────────────────────────────────────
void UTBGameplayAbility::FinishAbility()
{
	if (bAbilityFinished) return;
	bAbilityFinished = true;

	ABattleManager* BM = GetBattleManager();
	AActor* Avatar = IsValid(GetAvatarActorFromActorInfo()) ? CurrentActorInfo->AvatarActor.Get() : nullptr;

	if (!BM)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	if (IsValid(Avatar))
	{
		// Melee: 위치 복귀
		if (AnimationType == EAbilityAnimType::Melee && OriginLocation != FVector::ZeroVector)
		{
			Avatar->SetActorLocation(OriginLocation);
		}

		// 회전 복귀 (BattleManager::ExecuteAction에서 저장한 원래 회전)
		if (ABattleCombatant* Combatant = Cast<ABattleCombatant>(Avatar))
		{
			Avatar->SetActorRotation(Combatant->PreAbilityRotation);
		}
	}

	BM->OnActionComplete();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

// ─── Impact 액터 ──────────────────────────────────────────────────────────────
void UTBGameplayAbility::SpawnImpactActor()
{
	if (!ImpactActorClass) return;

	ABattleManager* BM = GetBattleManager();
	AActor* Avatar = CurrentActorInfo->AvatarActor.Get();

	if (!Avatar || !BM) return;

	const ABattleImpactActor* CDO = ImpactActorClass->GetDefaultObject<ABattleImpactActor>();

	// 전체 공격(AllEnemies/AllAllies)일 때 스폰 위치 결정
	TArray<ABattleCombatant*> SpawnTargets;
	bool bIsAllTargetType = (TargetType == EAbilityTargetType::AllEnemies || TargetType == EAbilityTargetType::AllAllies);

	if (bIsAllTargetType)
	{
		if (TargetType == EAbilityTargetType::AllEnemies)
			SpawnTargets = BM->GetLivingEnemies();
		else
			SpawnTargets = BM->GetLivingPlayers();

		if (SpawnTargets.IsEmpty()) return;

		if (bSpawnImpactPerTarget)
		{
			// 각 타겟마다 ImpactActor 스폰 (개별 타겟 모드)
			for (int32 i = 0; i < SpawnTargets.Num(); ++i)
			{
				ABattleCombatant* Target = SpawnTargets[i];
				if (!Target) continue;

				SpawnSingleImpactActor(Avatar, Target, CDO, i == 0, true); // 첫 번째만 OnFinished 바인딩, 개별 타겟 모드
			}
			return;
		}
		else
		{
			// 모든 타겟의 중간 위치에 하나만 스폰
			FVector CenterLocation = FVector::ZeroVector;
			for (ABattleCombatant* T : SpawnTargets)
			{
				if (T) CenterLocation += T->GetActorLocation();
			}
			CenterLocation /= SpawnTargets.Num();

			// 중간 위치에 가상 타겟처럼 스폰 (PendingTarget 대신 사용)
			SpawnSingleImpactActorAtLocation(Avatar, CenterLocation, CDO, true);
			return;
		}
	}

	// 단일 타겟 (기존 로직)
	ABattleCombatant* Target = BM->GetPendingTarget();
	if (!Target) return;

	SpawnSingleImpactActor(Avatar, Target, CDO, true);
}

void UTBGameplayAbility::SpawnSingleImpactActor(AActor* Avatar, ABattleCombatant* Target, const ABattleImpactActor* CDO, bool bBindFinished, bool bPerTargetMode)
{
	if (!Target) return;

	FVector FinalSpawnLocation;
	FRotator FinalSpawnRotation;

	if (AnimationType == EAbilityAnimType::Impact)
	{
		FinalSpawnLocation = Target->GetActorLocation() + (CDO ? CDO->SpawnOffset : FVector::ZeroVector);
		FinalSpawnRotation = CDO ? CDO->GetActorRotation() : FRotator(-90.f, 0.f, 0.f);
	}
	else // Ranged 타입
	{
		FinalSpawnLocation = Avatar->GetActorLocation() + Avatar->GetActorRotation().RotateVector(CDO ? CDO->SpawnOffset : FVector::ZeroVector);
		FinalSpawnRotation = (Target->GetActorLocation() - FinalSpawnLocation).Rotation();
	}

	ABattleImpactActor* ImpactActor = GetWorld()->SpawnActorDeferred<ABattleImpactActor>(
		ImpactActorClass, FTransform(FinalSpawnRotation, FinalSpawnLocation), nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (ImpactActor)
	{
		if (AnimationType != EAbilityAnimType::Impact)
		{
			// Ranged 타입: 발사 방향 설정 + 거리 기반 ImpactDelay 자동 계산 활성
			ImpactActor->ShootDirection = FinalSpawnRotation.Vector();
			ImpactActor->TargetLocation = Target->GetActorLocation();
			ImpactActor->bAutoCalculateImpactDelay = true;
		}

		// 개별 타겟 모드: OnImpact 대신 직접 타겟에 데미지 적용
		if (bPerTargetMode)
		{
			ImpactActor->SetPerTargetMode(this, Target, PendingHitIndex);
		}
		else
		{
			ImpactActor->OnImpact.AddDynamic(this, &UTBGameplayAbility::OnImpactNotify);
		}

		if (AnimationType == EAbilityAnimType::Impact && bBindFinished)
		{
			ImpactActor->OnFinished.AddDynamic(this, &UTBGameplayAbility::OnImpactFinished);
		}

		// Deferred Spawn 완료 (BeginPlay 호출)
		ImpactActor->FinishSpawning(FTransform(FinalSpawnRotation, FinalSpawnLocation));
	}
}

void UTBGameplayAbility::SpawnSingleImpactActorAtLocation(AActor* Avatar, const FVector& Location, const ABattleImpactActor* CDO, bool bBindFinished)
{
	FVector FinalSpawnLocation = Location + (CDO ? CDO->SpawnOffset : FVector::ZeroVector);
	FRotator FinalSpawnRotation = CDO ? CDO->GetActorRotation() : FRotator(-90.f, 0.f, 0.f);

	ABattleImpactActor* ImpactActor = GetWorld()->SpawnActorDeferred<ABattleImpactActor>(
		ImpactActorClass, FTransform(FinalSpawnRotation, FinalSpawnLocation), nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (ImpactActor)
	{
		ImpactActor->OnImpact.AddDynamic(this, &UTBGameplayAbility::OnImpactNotify);

		if (bBindFinished)
		{
			ImpactActor->OnFinished.AddDynamic(this, &UTBGameplayAbility::OnImpactFinished);
		}

		ImpactActor->FinishSpawning(FTransform(FinalSpawnRotation, FinalSpawnLocation));
	}
}

void UTBGameplayAbility::OnImpactNotify()
{
	ApplyDamage(PendingHitIndex);
}

// ─── 개별 타겟에만 데미지 적용 (bSpawnImpactPerTarget 모드용) ──────────────────
void UTBGameplayAbility::ApplyDamageToSingleTarget(ABattleCombatant* Target, int32 HitIndex)
{
	if (!DamageEffectClass || !Target) return;

	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	if (!TargetASC) return;

	const float Multiplier = HitMultipliers.IsValidIndex(HitIndex) ? HitMultipliers[HitIndex] : 1.0f;

	// 데미지 GE 적용
	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
		CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageEffectClass, 1.f);

	if (AbilityTypeTag.IsValid())
		SpecHandle.Data->CapturedSourceTags.GetSpecTags().AddTag(AbilityTypeTag);

	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_AbilityMultiplier, Multiplier);
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	// 상태이상 부여
	if (!OnHitStatusEffects.IsEmpty())
	{
		ABattleCombatant* Caster = Cast<ABattleCombatant>(CurrentActorInfo->AvatarActor.Get());
		const float CasterMagic = Caster ? Caster->GetMagicAttack() : 0.f;

		for (const FStatusEffectConfig& Config : OnHitStatusEffects)
		{
			if (!Config.StatusTag.IsValid() || Config.StacksToApply <= 0) continue;

			FStatusEffectInstance Instance;
			Instance.StatusTag       = Config.StatusTag;
			Instance.MagnitudePerStack = CasterMagic * Config.ScalingCoeff;
			Instance.RemainingTurns  = Config.StacksToApply;

			Target->ApplyStatusEffect(Instance);
		}
	}
}

void UTBGameplayAbility::OnImpactFinished()
{
	FinishAbility();
}

// ─── 내부 유틸 ────────────────────────────────────────────────────────────────
ABattleManager* UTBGameplayAbility::GetBattleManager() const
{
	UWorld* World = CurrentActorInfo && CurrentActorInfo->AvatarActor.IsValid()
		? CurrentActorInfo->AvatarActor->GetWorld() : nullptr;
	if (!World) return nullptr;

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(World, ABattleManager::StaticClass(), Found);
	return Found.IsEmpty() ? nullptr : Cast<ABattleManager>(Found[0]);
}

bool UTBGameplayAbility::CheckCost(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
		return false;
	if (ActorInfo)
		return CanAffordCost(ActorInfo->AbilitySystemComponent.Get());
	return true;
}

void UTBGameplayAbility::RequestSpawnImpact(int32 HitIndex)
{
	bImpactSpawned = true;
	PendingHitIndex = HitIndex;
	SpawnImpactActor();
}

bool UTBGameplayAbility::CanAffordCost(UAbilitySystemComponent* ASC) const
{
	if (!ASC || CostType == EAbilityCostType::None || CostAmount <= 0.f)
		return true;

	const UTBAttributeSet* Attrs = ASC->GetSet<UTBAttributeSet>();
	if (!Attrs)
		return false;

	switch (CostType)
	{
	case EAbilityCostType::Stamina: return Attrs->GetStamina() >= CostAmount;
	case EAbilityCostType::MP:      return Attrs->GetMP()      >= CostAmount;
	case EAbilityCostType::HP:      return Attrs->GetHP()      >  CostAmount;
	default:                        return true;
	}
}

// ─── Hit 이펙트 스폰 (AnimNotify_AbilityEffect에서 호출) ─────────────────────────
void UTBGameplayAbility::SpawnHitEffect()
{
	if (!HitEffect) return;

	AActor* Avatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar) return;

	// 플레이어 위치 + 오프셋 (로컬 방향 기준)
	const FVector SpawnLocation = Avatar->GetActorLocation() + Avatar->GetActorRotation().RotateVector(HitEffectOffset);
	const FRotator SpawnRotation = Avatar->GetActorRotation();

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(), HitEffect, SpawnLocation, SpawnRotation);
}

// ─── 컷씬 카메라 처리 ─────────────────────────────────────────────────────────────
void UTBGameplayAbility::StartCutsceneCamera()
{
	AActor* Avatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr;
	ABattleManager* BM = GetBattleManager();
	if (!Avatar || !BM)
	{
		// 컷씬 스킵하고 바로 몽타주 재생
		DoPlayMontage();
		return;
	}

	// 컷씬 카메라 위치 계산 (시전자 로컬 기준)
	const FTransform AvatarTransform = Avatar->GetActorTransform();
	const FVector CutsceneWorldLocation = AvatarTransform.TransformPosition(CutsceneCameraLocalOffset);
	const FRotator CutsceneWorldRotation = AvatarTransform.TransformRotation(
		CutsceneCameraLocalRotation.Quaternion()).Rotator();

	// 카메라 이동 시작
	BM->SetActionCameraWorldPosition(CutsceneWorldLocation, CutsceneWorldRotation, CutsceneBlendInTime);

	// 카메라 블렌딩 완료 후 컷씬 시작 (Niagara, 홀드 타이머 등)
	Avatar->GetWorldTimerManager().SetTimer(
		CutsceneCameraBlendTimer, this, &UTBGameplayAbility::OnCutsceneCameraBlendFinished, CutsceneBlendInTime, false);
}

void UTBGameplayAbility::OnCutsceneCameraBlendFinished()
{
	AActor* Avatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar) return;

	// Niagara 스폰 타이머 설정 (카메라 블렌딩 완료 시점 기준)
	if (CutsceneNiagaraEffect)
	{
		Avatar->GetWorldTimerManager().SetTimer(
			CutsceneNiagaraTimer, this, &UTBGameplayAbility::SpawnCutsceneNiagara, CutsceneNiagaraDelay, false);
	}

	// 컷씬 종료 타이머 설정 (카메라 블렌딩 완료 후부터 CutsceneDuration 만큼 유지)
	Avatar->GetWorldTimerManager().SetTimer(
		CutsceneTimer, this, &UTBGameplayAbility::OnCutsceneFinished, CutsceneDuration, false);
}

void UTBGameplayAbility::SpawnCutsceneNiagara()
{
	if (!CutsceneNiagaraEffect) return;

	AActor* Avatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar) return;

	// Niagara 스폰 위치 계산 (시전자 로컬 기준)
	const FVector SpawnLocation = Avatar->GetActorLocation() +
		Avatar->GetActorRotation().RotateVector(CutsceneNiagaraLocalOffset);
	const FRotator SpawnRotation = Avatar->GetActorRotation();

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(), CutsceneNiagaraEffect, SpawnLocation, SpawnRotation);
}

void UTBGameplayAbility::OnCutsceneFinished()
{
	// 컷씬 종료 후 액션 카메라로 전환 → ImpactActor 스폰
	SwitchToActionCameraAndSpawnImpact();
}

void UTBGameplayAbility::SwitchToActionCameraAndSpawnImpact()
{
	AActor* Avatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr;
	ABattleManager* BM = GetBattleManager();

	if (Avatar && BM)
	{
		// 캐릭터 현재 회전 기준으로 카메라 위치 계산 (캐릭터 회전에 맞춰 카메라도 회전)
		const FTransform AvatarTransform = Avatar->GetActorTransform();
		const FVector ActionWorldLocation = AvatarTransform.TransformPosition(ActionCameraLocalOffset);
		const FRotator ActionWorldRotation = AvatarTransform.TransformRotation(
			ActionCameraLocalRotation.Quaternion()).Rotator();

		BM->SetActionCameraWorldPosition(ActionWorldLocation, ActionWorldRotation, CameraBlendInTime);
	}

	// ImpactActor 스폰
	SpawnImpactActor();
}
