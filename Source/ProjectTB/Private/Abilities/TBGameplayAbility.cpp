#include "Abilities/TBGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Attributes/TBAttributeSet.h"
#include "Battle/BattleManager.h"
#include "Battle/BattleCombatant.h"
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
	OriginLocation = FVector::ZeroVector;

	AActor* Avatar = CurrentActorInfo->AvatarActor.Get();
	if (Avatar)
	{
		// 원래 회전값 저장
		OriginRotation = Avatar->GetActorRotation();
	}

	// ─── 시전자가 타겟을 바라보게 회전 ───
	ABattleManager* BM = GetBattleManager();

	if (Avatar && BM)
	{
		FVector LookAtLocation = FVector::ZeroVector;

		// AllEnemies/AllAllies는 모든 타겟의 중앙을 바라봄
		if (TargetType == EAbilityTargetType::AllEnemies)
		{
			TArray<ABattleCombatant*> Enemies = BM->GetLivingEnemies();
			for (ABattleCombatant* E : Enemies)
				if (E) LookAtLocation += E->GetActorLocation();
			if (!Enemies.IsEmpty()) LookAtLocation /= Enemies.Num();
		}
		else if (TargetType == EAbilityTargetType::AllAllies)
		{
			TArray<ABattleCombatant*> Allies = BM->GetLivingPlayers();
			for (ABattleCombatant* A : Allies)
				if (A) LookAtLocation += A->GetActorLocation();
			if (!Allies.IsEmpty()) LookAtLocation /= Allies.Num();
		}
		else if (ABattleCombatant* Target = BM->GetPendingTarget())
		{
			LookAtLocation = Target->GetActorLocation();
		}

		// 높이(Z) 차이를 무시하고 수평 방향으로만 회전
		FVector Dir = LookAtLocation - Avatar->GetActorLocation();
		Dir.Z = 0.f;

		if (!Dir.IsNearlyZero())
		{
			Avatar->SetActorRotation(Dir.Rotation());
		}
	}

	// Melee: 0.5초 후 순간이동
	if (AnimationType == EAbilityAnimType::Melee)
	{
		if (Avatar)
		{
			Avatar->GetWorldTimerManager().SetTimer(PreMoveTimer, this, &UTBGameplayAbility::DoTeleportToTarget, 0.5f, false);
		}
		return;
	}

	// Ranged / Impact: 바로 몽타주 재생
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
		SpawnImpactActor();
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
	if (!DamageEffectClass) return;

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
	case EAbilityTargetType::Self:
		Targets = BM->GetLivingPlayers();
		break;
	case EAbilityTargetType::SingleEnemy:
	case EAbilityTargetType::SingleAlly:
	default:
		if (ABattleCombatant* SingleTarget = BM->GetPendingTarget())
			Targets.Add(SingleTarget);
		break;
	}

	// 모든 타겟에 GE 적용
	for (ABattleCombatant* Target : Targets)
	{
		if (!Target) continue;

		UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
		if (!TargetASC) continue;

		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
			CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageEffectClass, 1.f);

		if (AbilityTypeTag.IsValid())
			SpecHandle.Data->CapturedSourceTags.GetSpecTags().AddTag(AbilityTypeTag);

		SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_AbilityMultiplier, Multiplier);
		TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

// ─── 어빌리티 종료 ────────────────────────────────────────────────────────────
void UTBGameplayAbility::FinishAbility()
{
	if (bAbilityFinished) return;
	bAbilityFinished = true;

	ABattleManager* BM = GetBattleManager();
	AActor* Avatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr;

	if (!BM)
	{
		UE_LOG(LogTemp, Warning, TEXT("TBGameplayAbility: BattleManager를 찾을 수 없습니다."));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	if (Avatar)
	{
		// Melee: 위치 복귀
		if (AnimationType == EAbilityAnimType::Melee && OriginLocation != FVector::ZeroVector)
		{
			Avatar->SetActorLocation(OriginLocation);
		}

		// 회전 복귀
		Avatar->SetActorRotation(OriginRotation);
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
			// 각 타겟마다 ImpactActor 스폰
			for (int32 i = 0; i < SpawnTargets.Num(); ++i)
			{
				ABattleCombatant* Target = SpawnTargets[i];
				if (!Target) continue;

				SpawnSingleImpactActor(Avatar, Target, CDO, i == 0); // 첫 번째만 OnFinished 바인딩
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

void UTBGameplayAbility::SpawnSingleImpactActor(AActor* Avatar, ABattleCombatant* Target, const ABattleImpactActor* CDO, bool bBindFinished)
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
			ImpactActor->ShootDirection = FinalSpawnRotation.Vector();
		}

		ImpactActor->OnImpact.AddDynamic(this, &UTBGameplayAbility::OnImpactNotify);

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
