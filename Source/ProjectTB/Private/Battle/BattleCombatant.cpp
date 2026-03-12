#include "Battle/BattleCombatant.h"
#include "AbilitySystemComponent.h"
#include "Attributes/TBAttributeSet.h"
#include "Abilities/TBGameplayAbility.h"
#include "TBGameplayTags.h"
#include "Components/WidgetComponent.h"
#include "UI/DamageNumberWidget.h"
#include "UI/StatusIconPanelWidget.h"
#include "GameplayEffect.h"
#include "Battle/BattleManager.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

ABattleCombatant::ABattleCombatant()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet           = CreateDefaultSubobject<UTBAttributeSet>(TEXT("AttributeSet"));

	// ?��?지 ?�젯???�커 컴포?�트 (?�젯?� ?��??�에 ?�적 ?�성)
	DamageWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("DamageWidget"));
	DamageWidgetComponent->SetupAttachment(GetRootComponent());
	DamageWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	DamageWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	DamageWidgetComponent->SetVisibility(false);

	TargetIndicatorComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("TargetIndicator"));
	TargetIndicatorComponent->SetupAttachment(GetRootComponent());
	TargetIndicatorComponent->SetRelativeLocation(FVector(0.f, 0.f, 200.f)); // 머리 ??
	TargetIndicatorComponent->SetWidgetSpace(EWidgetSpace::Screen);
	TargetIndicatorComponent->SetVisibility(false);

	StatusIconComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("StatusIconPanel"));
	StatusIconComponent->SetupAttachment(GetRootComponent());
	StatusIconComponent->SetRelativeLocation(FVector(0.f, 0.f, 240.f)); // ?�겟인?��??�터 ??
	StatusIconComponent->SetWidgetSpace(EWidgetSpace::Screen);
	StatusIconComponent->SetDrawAtDesiredSize(true);
	StatusIconComponent->SetPivot(FVector2D(0.5f, 1.0f)); // 가�?중앙, ?�로 ?�단 기�?
	StatusIconComponent->SetVisibility(false);
}

UAbilitySystemComponent* ABattleCombatant::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABattleCombatant::BeginPlay()
{
	Super::BeginPlay();
	InitAbilitySystem();

	// StatusIconPanel ?�젯 초기??�??�리게이??바인??
	if (StatusIconComponent && StatusIconWidgetClass)
	{
		StatusIconComponent->SetWidgetClass(StatusIconWidgetClass);
		StatusIconComponent->InitWidget();
		if (UStatusIconPanelWidget* Panel = Cast<UStatusIconPanelWidget>(StatusIconComponent->GetUserWidgetObject()))
			Panel->InitWithCombatant(this);
	}
}

void ABattleCombatant::InitAbilitySystem()
{
	if (bAbilitySystemInitialized || !AbilitySystemComponent) return;

	// GAS ?�성?? ?��??�레?�어: Owner = Avatar = this
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	ApplyStartingEffects();    // 초기 ?�탯 ?�팅 (HP=300, Speed=80 ??
	GrantStartingAbilities();  // ?�빌리티 부??

	bAbilitySystemInitialized = true;
}

void ABattleCombatant::ApplyStartingEffects()
{
	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	for (const TSubclassOf<UGameplayEffect>& EffectClass : StartingEffects)
	{
		if (!EffectClass) continue;
		FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, Context);
		if (Spec.IsValid())
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void ABattleCombatant::GrantStartingAbilities()
{
	for (const TSubclassOf<UTBGameplayAbility>& AbilityClass : StartingAbilities)
	{
		if (AbilityClass)
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
	}
}

// ?�?�?� ?�트리뷰???�근???�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�
float ABattleCombatant::GetHP()         const { return AttributeSet ? AttributeSet->GetHP()         : 0.f; }
float ABattleCombatant::GetMaxHP()      const { return AttributeSet ? AttributeSet->GetMaxHP()      : 0.f; }
float ABattleCombatant::GetMP()         const { return AttributeSet ? AttributeSet->GetMP()         : 0.f; }
float ABattleCombatant::GetMaxMP()      const { return AttributeSet ? AttributeSet->GetMaxMP()      : 0.f; }
float ABattleCombatant::GetStamina()    const { return AttributeSet ? AttributeSet->GetStamina()    : 0.f; }
float ABattleCombatant::GetMaxStamina() const { return AttributeSet ? AttributeSet->GetMaxStamina() : 0.f; }
float ABattleCombatant::GetMagicAttack() const { return AttributeSet ? AttributeSet->GetMagicAttack() : 0.f; }
float ABattleCombatant::GetSpeed() const
{
	float Base = AttributeSet ? AttributeSet->GetSpeed() : 0.f;
	// ???�태 ??Speed 30% 감소 (?�음 ?�운??BuildRoundOrder??반영)
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(TAG_Status_Poison))
		Base *= 0.7f;
	return Base;
}
bool  ABattleCombatant::IsDead()        const { return GetHP() <= 0.f; }

TArray<UTBGameplayAbility*> ABattleCombatant::GetGrantedAbilities() const
{
	TArray<UTBGameplayAbility*> Result;
	if (!AbilitySystemComponent) return Result;

	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		// ?�스?�스가 ?�으�??�스?�스, ?�으�?CDO 반환
		if (UTBGameplayAbility* Instance = Cast<UTBGameplayAbility>(Spec.GetPrimaryInstance()))
			Result.Add(Instance);
		else if (UTBGameplayAbility* CDO = Cast<UTBGameplayAbility>(Spec.Ability.Get()))
			Result.Add(CDO);
	}
	return Result;
}

// ?�?�?� AnimNotify ?��?지 ?�결 ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�
void ABattleCombatant::OnHitNotify(int32 HitIndex)
{
	if (!AbilitySystemComponent) return;

	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (UTBGameplayAbility* Instance = Cast<UTBGameplayAbility>(Spec.GetPrimaryInstance()))
		{
			if (Instance->IsActive())
			{
				Instance->ApplyDamage(HitIndex);
				return;
			}
		}
	}
}

void ABattleCombatant::OnSpawnImpactNotify(int32 HitIndex)
{
	if (!AbilitySystemComponent) return;

	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (UTBGameplayAbility* Instance = Cast<UTBGameplayAbility>(Spec.GetPrimaryInstance()))
		{
			if (Instance->IsActive())
			{
				// ?�빌리티???�폰 ?�수 ?�출
				Instance->RequestSpawnImpact(HitIndex);
				return;
			}
		}
	}
}

void ABattleCombatant::OnOpenParryTimingNotify(float Duration)
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABattleManager::StaticClass(), Found);
	if (!Found.IsEmpty())
	{
		if (ABattleManager* BM = Cast<ABattleManager>(Found[0]))
			BM->OpenParryTiming(Duration);
	}
}

void ABattleCombatant::PlayParryMontage()
{
	if (ParryMontage)
		PlayAnimMontage(ParryMontage);

	// ?�링 ?�펙???�폰
	if (ParryEffect)
	{
		const FVector SpawnLocation = GetActorLocation() + GetActorRotation().RotateVector(ParryEffectOffset);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ParryEffect, SpawnLocation, GetActorRotation());
	}
}

// ?�?�?� 방어 ?�태 ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�
void ABattleCombatant::SetDefending(bool bDefending)
{
	if (!AbilitySystemComponent) return;
	if (bDefending)
		AbilitySystemComponent->AddLooseGameplayTag(TAG_Combatant_State_Defending);
	else
		AbilitySystemComponent->RemoveLooseGameplayTag(TAG_Combatant_State_Defending);

	// ?�이�?UI 갱신 ?�리�?(방어 ?�이�??�시/?�제)
	OnStatusEffectsChanged.Broadcast(this);
}

bool ABattleCombatant::IsDefending() const
{
	if (!AbilitySystemComponent) return false;
	return AbilitySystemComponent->HasMatchingGameplayTag(TAG_Combatant_State_Defending);
}

// ?�?�?� ?�망 처리 (AttributeSet?�서 ?�출) ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�
void ABattleCombatant::OnDeathInternal()
{
	AbilitySystemComponent->AddLooseGameplayTag(TAG_Combatant_State_Dead);
	AbilitySystemComponent->CancelAllAbilities();
	OnDeath.Broadcast(this);	

	// 0.5�???Actor ?�거
	FTimerHandle DestroyTimer;
	GetWorldTimerManager().SetTimer(DestroyTimer, this, &ABattleCombatant::DestroyAfterDeath, 0.5f, false);
}

void ABattleCombatant::DestroyAfterDeath()
{
	Destroy();
}

void ABattleCombatant::OnDamageReceivedInternal(float Damage, bool bIsCritical)
{
	OnDamageReceived.Broadcast(this, Damage, bIsCritical);
	SpawnDamageNumber(Damage, false, bIsCritical);
}

void ABattleCombatant::OnStatChangedInternal()
{
	OnStatChanged.Broadcast(this);
}

void ABattleCombatant::OnHealReceivedInternal(float Heal)
{
	OnHealReceived.Broadcast(this, Heal);
	SpawnDamageNumber(Heal, true);
}

void ABattleCombatant::SpawnDamageNumber(float Value, bool bIsHeal, bool bIsCritical)
{
	if (!DamageNumberWidgetClass) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	// 기존 ?�젯?�을 ?�로 밀?�올�?
	for (UDamageNumberWidget* ExistingWidget : ActiveDamageNumbers)
	{
		if (ExistingWidget)
		{
			ExistingWidget->MoveUp(DamageNumberStackSpacing);
		}
	}

	// ???�젯 ?�성
	UDamageNumberWidget* NewWidget = CreateWidget<UDamageNumberWidget>(PC, DamageNumberWidgetClass);
	if (!NewWidget) return;

	// ?��?지/??�??�정
	if (bIsHeal)
	{
		NewWidget->SetHeal(Value);
	}
	else
	{
		NewWidget->SetDamage(Value, bIsCritical);
	}

	// 캐릭?��? ?�라가?�록 ?�정 (카메???�동 ?�??
	NewWidget->SetFollowTarget(this, 100.f);

	// Viewport??추�?
	NewWidget->AddToViewport(100);

	// ?�명 ?�?�머 ?�작
	NewWidget->StartLifespan(1.5f);

	// ?�료 ?�리게이??바인??
	NewWidget->OnFinished.BindLambda([this, NewWidget]()
	{
		OnDamageNumberRemoved(NewWidget);
	});

	ActiveDamageNumbers.Add(NewWidget);
}

void ABattleCombatant::OnDamageNumberRemoved(UDamageNumberWidget* Widget)
{
	ActiveDamageNumbers.Remove(Widget);
}

void ABattleCombatant::ShowTargetIndicator()
{
	if (!IsValid(this) || !TargetIndicatorComponent) return;

	// ?�젯 ?�래?��? 블루?�린?�에???�당?�었?��? ?�인
	if (TargetIndicatorWidgetClass)
	{
		// ?��? ?�젯???�정?�어 ?�는지 ?�인 ???�정
		if (TargetIndicatorComponent->GetWidgetClass() != TargetIndicatorWidgetClass)
		{
			TargetIndicatorComponent->SetWidgetClass(TargetIndicatorWidgetClass);
		}
	}

	TargetIndicatorComponent->SetVisibility(true);
}

void ABattleCombatant::HideTargetIndicator()
{
	if (TargetIndicatorComponent)
		TargetIndicatorComponent->SetVisibility(false);
}

// ?�?�?� ?�태?�상 ?�스???�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�

void ABattleCombatant::ApplyStatusEffect(const FStatusEffectInstance& NewEffect)
{
	if (!NewEffect.StatusTag.IsValid() || NewEffect.RemainingTurns <= 0) return;

	ActiveStatusEffects.Add(NewEffect);

	// GAS Loose Tag 즉시 반영 (?�으�?추�?)
	if (AbilitySystemComponent && !AbilitySystemComponent->HasMatchingGameplayTag(NewEffect.StatusTag))
		AbilitySystemComponent->AddLooseGameplayTag(NewEffect.StatusTag);

	// ?�태?�상 ?�이�??�널 ?�성??
	if (StatusIconComponent)
	{
		if (StatusIconWidgetClass && !StatusIconComponent->GetWidget())
			StatusIconComponent->SetWidgetClass(StatusIconWidgetClass);
		StatusIconComponent->SetVisibility(true);
	}

	OnStatusEffectsChanged.Broadcast(this);
}

bool ABattleCombatant::TickStatusEffects()
{
	if (ActiveStatusEffects.IsEmpty()) return false;

	bool bWasStunned = false;
	bool bChanged    = false;

	// ??�� ?�회 (RemoveAt ???�전)
	for (int32 i = ActiveStatusEffects.Num() - 1; i >= 0; --i)
	{
		FStatusEffectInstance& Effect = ActiveStatusEffects[i];

		if (Effect.StatusTag == TAG_Status_Stun)
		{
			// ?�턴: ?��?지 ?�이 ???�킵 ?�시
			bWasStunned = true;
		}
		else if (Effect.StatusTag == TAG_Status_Burn)
		{
			// ?�상: ?�탯 기반 ?��?지 (방어??무시)
			ApplyStatusTickDamage(FMath::Max(1.f, Effect.MagnitudePerStack));
		}
		else if (Effect.StatusTag == TAG_Status_Poison)
		{
			// ?? ?�재 HP 1% + ?�탯 기반 ?��?지 (방어??무시)
			const float HPPercent = GetHP() * 0.01f;
			ApplyStatusTickDamage(FMath::Max(1.f, HPPercent + Effect.MagnitudePerStack));
		}
		else if (Effect.StatusTag == TAG_Status_Regen)
		{
			// ?�생: ??
			ApplyStatusTickHeal(FMath::Max(1.f, Effect.MagnitudePerStack));
		}

		// ?�택 1 ?�모
		Effect.RemainingTurns--;
		bChanged = true; // ?�택 변�?????�� UI 갱신
		if (Effect.RemainingTurns <= 0)
			ActiveStatusEffects.RemoveAt(i);
	}

	SyncStatusTags();

	// 모든 ?�과가 ?�진?�면 ?�이�??�널 ?��?
	if (ActiveStatusEffects.IsEmpty() && StatusIconComponent)
		StatusIconComponent->SetVisibility(false);

	if (bChanged || bWasStunned)
		OnStatusEffectsChanged.Broadcast(this);

	return bWasStunned;
}

void ABattleCombatant::SyncStatusTags()
{
	if (!AbilitySystemComponent) return;

	// ?�재 ActiveStatusEffects???�는 ?�그 목록 ?�집
	TSet<FGameplayTag> WantedTags;
	for (const FStatusEffectInstance& E : ActiveStatusEffects)
		WantedTags.Add(E.StatusTag);

	// 관�??�??Status ?�그??
	const FGameplayTag StatusTags[] = {
		TAG_Status_Burn, TAG_Status_Poison, TAG_Status_Regen, TAG_Status_Stun
	};

	for (const FGameplayTag& Tag : StatusTags)
	{
		const bool bShouldHave  = WantedTags.Contains(Tag);
		const bool bCurrentlyHas = AbilitySystemComponent->HasMatchingGameplayTag(Tag);

		if (bShouldHave && !bCurrentlyHas)
			AbilitySystemComponent->AddLooseGameplayTag(Tag);
		else if (!bShouldHave && bCurrentlyHas)
			AbilitySystemComponent->RemoveLooseGameplayTag(Tag);
	}
}

void ABattleCombatant::ApplyStatusTickDamage(float Damage)
{
	if (!AbilitySystemComponent || Damage <= 0.f) return;

	// ?�적 GE�?IncomingDamage 메�? ?�트리뷰?�에 직접 주입
	// ??PostGameplayEffectExecute?�서 HP 차감 + ?��?지 ?�자 UI + ?�망 처리 ?�동 ?�결
	UGameplayEffect* GEObj = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None);
	GEObj->DurationPolicy = EGameplayEffectDurationType::Instant;

	// IncomingDamage ?�정
	FGameplayModifierInfo DamageMod;
	DamageMod.Attribute        = UTBAttributeSet::GetIncomingDamageAttribute();
	DamageMod.ModifierOp       = EGameplayModOp::Additive;
	DamageMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Damage);
	GEObj->Modifiers.Add(DamageMod);

	// IncomingCritical???�정 (0 = 비크리티�? - PostGameplayEffectExecute ?�리거용
	FGameplayModifierInfo CritMod;
	CritMod.Attribute        = UTBAttributeSet::GetIncomingCriticalAttribute();
	CritMod.ModifierOp       = EGameplayModOp::Override;
	CritMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(0.f);
	GEObj->Modifiers.Add(CritMod);

	FGameplayEffectSpec Spec(GEObj, AbilitySystemComponent->MakeEffectContext(), 1.f);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(Spec);
}

void ABattleCombatant::ApplyStatusTickHeal(float Heal)
{
	if (!AbilitySystemComponent || Heal <= 0.f) return;

	UGameplayEffect* GEObj = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None);
	GEObj->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute        = UTBAttributeSet::GetIncomingHealAttribute();
	ModInfo.ModifierOp       = EGameplayModOp::Additive;
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(Heal);
	GEObj->Modifiers.Add(ModInfo);

	FGameplayEffectSpec Spec(GEObj, AbilitySystemComponent->MakeEffectContext(), 1.f);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(Spec);
}

// ?�?�?� ?�벨 ?�탯 ?�용 ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�

void ABattleCombatant::ApplyLevelStats(const FCharacterLevelStats& LevelStats, const FPartyMemberData& PartyData)
{
	if (!AttributeSet) return;

	// Max 스탯 적용 (DataTable 기반)
	AttributeSet->SetMaxHP(LevelStats.MaxHP);
	AttributeSet->SetMaxMP(LevelStats.MaxMP);
	AttributeSet->SetMaxStamina(LevelStats.MaxStamina);
	AttributeSet->SetPhysicalAttack(LevelStats.PhysicalAttack);
	AttributeSet->SetMagicAttack(LevelStats.MagicAttack);
	AttributeSet->SetPhysicalDefense(LevelStats.PhysicalDefense);
	AttributeSet->SetMagicDefense(LevelStats.MagicDefense);
	AttributeSet->SetSpeed(LevelStats.Speed);
	AttributeSet->SetCriticalChance(LevelStats.CriticalChance);
	AttributeSet->SetCriticalMultiplier(LevelStats.CriticalMultiplier);

	// 현재 스탯 적용 (GameInstance에서 저장된 값 또는 풀 상태)
	// -1이면 풀피/풀마나/풀스태로 시작
	const float HP = (PartyData.CurrentHP < 0.f) ? LevelStats.MaxHP : FMath::Min(PartyData.CurrentHP, LevelStats.MaxHP);
	const float MP = (PartyData.CurrentMP < 0.f) ? LevelStats.MaxMP : FMath::Min(PartyData.CurrentMP, LevelStats.MaxMP);
	const float Stamina = (PartyData.CurrentStamina < 0.f) ? LevelStats.MaxStamina : FMath::Min(PartyData.CurrentStamina, LevelStats.MaxStamina);

	AttributeSet->SetHP(HP);
	AttributeSet->SetMP(MP);
	AttributeSet->SetStamina(Stamina);
}

void ABattleCombatant::ApplyStatsDirectly(const FCharacterLevelStats& Stats)
{
	if (!AttributeSet) return;

	// 모든 ?�탯 직접 ?�용 (??캐릭?�용 - ?�재�?= 최�?�?
	AttributeSet->SetMaxHP(Stats.MaxHP);
	AttributeSet->SetHP(Stats.MaxHP);
	AttributeSet->SetMaxMP(Stats.MaxMP);
	AttributeSet->SetMP(Stats.MaxMP);
	AttributeSet->SetMaxStamina(Stats.MaxStamina);
	AttributeSet->SetStamina(Stats.MaxStamina);
	AttributeSet->SetPhysicalAttack(Stats.PhysicalAttack);
	AttributeSet->SetMagicAttack(Stats.MagicAttack);
	AttributeSet->SetPhysicalDefense(Stats.PhysicalDefense);
	AttributeSet->SetMagicDefense(Stats.MagicDefense);
	AttributeSet->SetSpeed(Stats.Speed);
	AttributeSet->SetCriticalChance(Stats.CriticalChance);
	AttributeSet->SetCriticalMultiplier(Stats.CriticalMultiplier);
}

// ?�?�?� ATB 게이지 ?�스???�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�

void ABattleCombatant::ChargeActionGauge()
{
	ActionGauge += GetSpeed();
}

void ABattleCombatant::ConsumeActionGauge()
{
	ActionGauge -= ActionGaugeThreshold;
	// ?�수 방�? (?�시 100 미만?�서 ?�동?�을 경우)
	if (ActionGauge < 0.f)
		ActionGauge = 0.f;
}








