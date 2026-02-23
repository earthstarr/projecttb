#include "Abilities/TBGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Attributes/TBAttributeSet.h"
#include "Battle/BattleManager.h"
#include "Battle/BattleCombatant.h"
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

	// Melee: 0.5초 후 순간이동
	if (AnimationType == EAbilityAnimType::Melee)
	{
		if (AActor* Avatar = ActorInfo->AvatarActor.Get())
			Avatar->GetWorldTimerManager().SetTimer(
				PreMoveTimer, this, &UTBGameplayAbility::DoTeleportToTarget, 0.5f, false);
		return;
	}

	// Ranged: 바로 몽타주 재생
	DoPlayMontage();
}

void UTBGameplayAbility::DoTeleportToTarget()
{
	AActor* Avatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar) { ApplyDamageAndFinish(); return; }

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(Avatar->GetWorld(), ABattleManager::StaticClass(), Found);
	ABattleManager* BM = Found.IsEmpty() ? nullptr : Cast<ABattleManager>(Found[0]);
	if (!BM) { ApplyDamageAndFinish(); return; }

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

	ApplyDamageAndFinish();
}

void UTBGameplayAbility::OnMontageBlendOut()
{
	// BlendOut 시작 시점에 데미지 적용 (타격감 있는 타이밍)
	ApplyDamageAndFinish();
}

void UTBGameplayAbility::OnMontageCompleted()
{
	// BlendOut이 없었을 경우 대비 (중단/취소 시)
	ApplyDamageAndFinish();
}

void UTBGameplayAbility::ApplyDamageAndFinish()
{
	if (bAbilityFinished) return;
	bAbilityFinished = true;

	// BattleManager 찾기
	UWorld* World = CurrentActorInfo && CurrentActorInfo->AvatarActor.IsValid()
		? CurrentActorInfo->AvatarActor->GetWorld() : nullptr;
	TArray<AActor*> Found;
	if (World) UGameplayStatics::GetAllActorsOfClass(World, ABattleManager::StaticClass(), Found);
	ABattleManager* BM = Found.IsEmpty() ? nullptr : Cast<ABattleManager>(Found[0]);

	if (!BM)
	{
		UE_LOG(LogTemp, Warning, TEXT("TBGameplayAbility: BattleManager를 찾을 수 없습니다."));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// PendingTarget에 GE 적용
	if (ABattleCombatant* Target = BM->GetPendingTarget())
	{
		if (UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent())
		{
			FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
				CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageEffectClass, 1.f);

			if (AbilityTypeTag.IsValid())
				SpecHandle.Data->CapturedSourceTags.GetSpecTags().AddTag(AbilityTypeTag);

			SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_AbilityMultiplier, AbilityMultiplier);
			TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	// Melee: 원위치 복귀
	if (AnimationType == EAbilityAnimType::Melee && CurrentActorInfo && CurrentActorInfo->AvatarActor.IsValid())
		if (OriginLocation != FVector::ZeroVector)
			CurrentActorInfo->AvatarActor->SetActorLocation(OriginLocation);

	BM->OnActionComplete();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
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
	case EAbilityCostType::HP:      return Attrs->GetHP()      >  CostAmount; // HP는 0 초과여야 함
	default:                        return true;
	}
}
