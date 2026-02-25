#include "Abilities/TBGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Attributes/TBAttributeSet.h"
#include "Battle/BattleManager.h"
#include "Battle/BattleCombatant.h"
#include "Battle/BattleImpactActor.h"
#include "Kismet/GameplayStatics.h"

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

	// ─── 시전자가 적을 바라보게 회전 ───
	ABattleManager* BM = GetBattleManager();
	ABattleCombatant* Target = BM ? BM->GetPendingTarget() : nullptr;

	if (Avatar && Target)
	{
		// 높이(Z) 차이를 무시하고 수평 방향으로만 회전 (캐릭터가 뒤집히지 않게)
		FVector Dir = Target->GetActorLocation() - Avatar->GetActorLocation();
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

	if (ABattleCombatant* Target = BM->GetPendingTarget())
	{
		OriginLocation = Avatar->GetActorLocation();

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

	ABattleCombatant* Target = BM->GetPendingTarget();
	if (!Target) return;

	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	if (!TargetASC) return;

	const float Multiplier = HitMultipliers.IsValidIndex(HitIndex) ? HitMultipliers[HitIndex] : 1.0f;

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
		CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageEffectClass, 1.f);

	if (AbilityTypeTag.IsValid())
		SpecHandle.Data->CapturedSourceTags.GetSpecTags().AddTag(AbilityTypeTag);

	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_AbilityMultiplier, Multiplier);
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
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
	ABattleCombatant* Target = BM ? BM->GetPendingTarget() : nullptr;
	AActor* Avatar = CurrentActorInfo->AvatarActor.Get();

	if (!Avatar || !Target) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const ABattleImpactActor* CDO = ImpactActorClass->GetDefaultObject<ABattleImpactActor>();

	FVector FinalSpawnLocation;
	FRotator FinalSpawnRotation;

	if (AnimationType == EAbilityAnimType::Impact)
	{
		// Impact 타입: 월드 절대 좌표 기준
		// 타겟 위치에 오프셋을 그대로 더함 (시전자의 회전값 무시)
		FinalSpawnLocation = Target->GetActorLocation() + (CDO ? CDO->SpawnOffset : FVector::ZeroVector);

		// 회전값은 액터에 설정된 기본값 사용
		FinalSpawnRotation = CDO ? CDO->GetActorRotation() : FRotator(-90.f, 0.f, 0.f);
	}
	else // Ranged 타입
	{
		// 원거리는 시전자의 손 위치 등 '방향'이 중요하므로 기존 로직 유지
		FinalSpawnLocation = Avatar->GetActorLocation() + Avatar->GetActorRotation().RotateVector(CDO ? CDO->SpawnOffset : FVector::ZeroVector);

		// 타겟을 향해 날아가도록 회전값 계산
		FinalSpawnRotation = (Target->GetActorLocation() - FinalSpawnLocation).Rotation();
	}

	// 스폰
	ABattleImpactActor* ImpactActor = GetWorld()->SpawnActor<ABattleImpactActor>(
	   ImpactActorClass, FinalSpawnLocation, FinalSpawnRotation, Params);

	if (ImpactActor)
	{
		// 발사 방향 설정 (액터의 전방 벡터)
		ImpactActor->ShootDirection = FinalSpawnRotation.Vector();

		ImpactActor->OnImpact.AddDynamic(this, &UTBGameplayAbility::OnImpactNotify);

		if (AnimationType == EAbilityAnimType::Impact)
		{
			ImpactActor->OnFinished.AddDynamic(this, &UTBGameplayAbility::OnImpactFinished);
		}
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
