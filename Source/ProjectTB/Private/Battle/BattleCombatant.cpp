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

    // 데미지 위젯용 앵커 컴포넌트 (위젯은 런타임에 동적 생성)
	DamageWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("DamageWidget"));
	DamageWidgetComponent->SetupAttachment(GetRootComponent());
	DamageWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	DamageWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	DamageWidgetComponent->SetVisibility(false);

	TargetIndicatorComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("TargetIndicator"));
	TargetIndicatorComponent->SetupAttachment(GetRootComponent());
    TargetIndicatorComponent->SetRelativeLocation(FVector(0.f, 0.f, 200.f)); // 머리 위
	TargetIndicatorComponent->SetWidgetSpace(EWidgetSpace::Screen);
	TargetIndicatorComponent->SetVisibility(false);

	StatusIconComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("StatusIconPanel"));
	StatusIconComponent->SetupAttachment(GetRootComponent());
    StatusIconComponent->SetRelativeLocation(FVector(0.f, 0.f, 240.f)); // 타겟인디케이터 위
	StatusIconComponent->SetWidgetSpace(EWidgetSpace::Screen);
	StatusIconComponent->SetDrawAtDesiredSize(true);
    StatusIconComponent->SetPivot(FVector2D(0.5f, 1.0f)); // 가로 중앙, 세로 하단 기준
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

    // StatusIconPanel 위젯 초기화 및 델리게이트 바인딩
	if (StatusIconComponent && StatusIconWidgetClass)
	{
		StatusIconComponent->SetWidgetClass(StatusIconWidgetClass);
		StatusIconComponent->InitWidget();
		if (UStatusIconPanelWidget* Panel = Cast<UStatusIconPanelWidget>(StatusIconComponent->GetUserWidgetObject()))
			Panel->InitWithCombatant(this);
	}
}

void ABattleCombatant::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 추가: 레벨 전환 시 데미지 숫자 위젯을 먼저 정리해 늦은 콜백이 죽은 Combatant를 건드리지 않게 한다.
	CleanupDamageNumbers();
	Super::EndPlay(EndPlayReason);
}

void ABattleCombatant::InitAbilitySystem()
{
	if (bAbilitySystemInitialized || !AbilitySystemComponent) return;

    // GAS 활성화, 싱글플레이어: Owner = Avatar = this
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

    ApplyStartingEffects();    // 초기 스탯 세팅 (HP=300, Speed=80 등)
    GrantStartingAbilities();  // 어빌리티 부여

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

// ─── 어트리뷰트 접근자 ────────────────────────────────────────────────────────
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
    // 독 상태 → Speed 30% 감소 (다음 라운드 BuildRoundOrder에 반영)
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
        // 인스턴스가 있으면 인스턴스, 없으면 CDO 반환
		if (UTBGameplayAbility* Instance = Cast<UTBGameplayAbility>(Spec.GetPrimaryInstance()))
			Result.Add(Instance);
		else if (UTBGameplayAbility* CDO = Cast<UTBGameplayAbility>(Spec.Ability.Get()))
			Result.Add(CDO);
	}
	return Result;
}

// ─── AnimNotify 데미지 연결 ────────────────────────────────────────────────────
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
                // 어빌리티의 스폰 함수 호출
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

    // 패링 이펙트 스폰
	if (ParryEffect)
	{
		const FVector SpawnLocation = GetActorLocation() + GetActorRotation().RotateVector(ParryEffectOffset);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ParryEffect, SpawnLocation, GetActorRotation());
	}
}

// ─── 방어 상태 ────────────────────────────────────────────────────────────────
void ABattleCombatant::SetDefending(bool bDefending)
{
	if (!AbilitySystemComponent) return;
	if (bDefending)
		AbilitySystemComponent->AddLooseGameplayTag(TAG_Combatant_State_Defending);
	else
		AbilitySystemComponent->RemoveLooseGameplayTag(TAG_Combatant_State_Defending);

	// StatusIconComponent Visibility 설정
	if (StatusIconComponent && bDefending)
	{
		StatusIconComponent->SetVisibility(true);
	}

    // 아이콘 UI 갱신 트리거 (방어 아이콘 표시/해제)
	OnStatusEffectsChanged.Broadcast(this);
}

bool ABattleCombatant::IsDefending() const
{
	if (!AbilitySystemComponent) return false;
	return AbilitySystemComponent->HasMatchingGameplayTag(TAG_Combatant_State_Defending);
}

// ─── 사망 처리 (AttributeSet에서 호출) ────────────────────────────────────────
void ABattleCombatant::OnDeathInternal()
{
	AbilitySystemComponent->AddLooseGameplayTag(TAG_Combatant_State_Dead);
	AbilitySystemComponent->CancelAllAbilities();
	OnDeath.Broadcast(this);

	// DeadMontage 재생
	if (DeadMontage)
	{
		PlayAnimMontage(DeadMontage);
	}

	// 상속 클래스에서 사망 처리 (플레이어: Hidden, 적: Destroy)
	HandleDeath();
}

void ABattleCombatant::HandleDeath()
{
	// 기본: 0.5초 후 Destroy (적 기본 동작)
	FTimerHandle DestroyTimer;
	GetWorldTimerManager().SetTimer(DestroyTimer, this, &ABattleCombatant::DestroyAfterDeath, 1.f, false);
}

void ABattleCombatant::DestroyAfterDeath()
{
	Destroy();
}

void ABattleCombatant::OnDamageReceivedInternal(float Damage, bool bIsCritical)
{
	OnDamageReceived.Broadcast(this, Damage, bIsCritical);
	SpawnDamageNumber(Damage, false, bIsCritical);

	// 패링 성공 시 HitMontage 재생 안함 (패링 몽타주가 이미 재생 중)
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(TAG_Combatant_State_ParrySuccess))
		return;

	// HitMontage 재생
	if (HitMontage)
	{
		PlayAnimMontage(HitMontage);
	}
}

void ABattleCombatant::OnStatChangedInternal()
{
	OnStatChanged.Broadcast(this);
}

bool ABattleCombatant::TryConsumeResurrectionStack()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (ASC == nullptr)
	{
		return false;
	}

	// 1차 확인: 실제 GAS 상태 기준으로 부활 권한이 있는가
	if (!ASC->HasMatchingGameplayTag(TAG_Artifact_Resurrection))
	{
		return false;
	}

	// 2차 확인: 캐시된 Traits로 실제 제거할 아티팩트 GE 찾기
	for (int32 i = AppliedArtifactEffects.Num() - 1; i >= 0; --i)
	{
		FAppliedArtifactEffect& Entry = AppliedArtifactEffects[i];

		if (!Entry.Handle.IsValid())
		{
			AppliedArtifactEffects.RemoveAt(i);
			continue;
		}

		if (!Entry.Traits.HasTagExact(TAG_Artifact_Resurrection))
		{
			continue;
		}

		if (ASC->RemoveActiveGameplayEffect(Entry.Handle, -1))
		{
			AppliedArtifactEffects.RemoveAt(i);
			return true;
		}
	}

	return false;
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

    // 기존 위젯들을 위로 밀어올림
	for (UDamageNumberWidget* ExistingWidget : ActiveDamageNumbers)
	{
		if (ExistingWidget)
		{
			ExistingWidget->MoveUp(DamageNumberStackSpacing);
		}
	}

    // 새 위젯 생성
	UDamageNumberWidget* NewWidget = CreateWidget<UDamageNumberWidget>(PC, DamageNumberWidgetClass);
	if (!NewWidget) return;

    // 데미지/힐 값 설정
	if (bIsHeal)
	{
		NewWidget->SetHeal(Value);
	}
	else
	{
		NewWidget->SetDamage(Value, bIsCritical);
	}

    // 캐릭터를 따라가도록 설정 (카메라 이동 대응)
	NewWidget->SetFollowTarget(this, 100.f);

    // Viewport에 추가
	NewWidget->AddToViewport(100);

	// 수명 타이머 시작
	NewWidget->StartLifespan(1.5f);

	// 변경: 기존 [this, NewWidget] raw 캡처는 전투 종료 후 파괴된 Combatant를 다시 참조할 수 있어 제거했다.
	// 추가: 약한 참조로 바꿔 Combatant나 위젯이 이미 정리됐으면 콜백을 조용히 무시한다.
	TWeakObjectPtr<ABattleCombatant> WeakThis(this);
	TWeakObjectPtr<UDamageNumberWidget> WeakWidget(NewWidget);
	NewWidget->OnFinished.BindLambda([WeakThis, WeakWidget]()
	{
		if (!WeakThis.IsValid())
		{
			return;
		}

		WeakThis->OnDamageNumberRemoved(WeakWidget.Get());
	});

	ActiveDamageNumbers.Add(NewWidget);
}

void ABattleCombatant::OnDamageNumberRemoved(UDamageNumberWidget* Widget)
{
	if (Widget == nullptr)
	{
		return;
	}

	ActiveDamageNumbers.Remove(Widget);
}

void ABattleCombatant::CleanupDamageNumbers()
{
	// 추가: 위젯 스스로 사라지기만 기다리던 기존 흐름 대신, 전환 시점에 델리게이트와 뷰포트를 즉시 정리한다.
	for (UDamageNumberWidget* Widget : ActiveDamageNumbers)
	{
		if (!IsValid(Widget))
		{
			continue;
		}

		Widget->OnFinished.Unbind();
		Widget->RemoveFromParent();
	}

	ActiveDamageNumbers.Reset();
}

void ABattleCombatant::ShowTargetIndicator()
{
	if (!IsValid(this) || !TargetIndicatorComponent) return;

    // 위젯 클래스가 블루프린트에서 할당되었는지 확인
	if (TargetIndicatorWidgetClass)
	{
        // 이미 위젯이 설정되어 있는지 확인 후 설정
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

// ─── 상태이상 시스템 ──────────────────────────────────────────────────────────

void ABattleCombatant::ApplyStatusEffect(const FStatusEffectInstance& NewEffect)
{
	if (!NewEffect.StatusTag.IsValid() || NewEffect.RemainingTurns <= 0) return;

	ActiveStatusEffects.Add(NewEffect);

    // GAS Loose Tag 즉시 반영 (없으면 추가)
	if (AbilitySystemComponent && !AbilitySystemComponent->HasMatchingGameplayTag(NewEffect.StatusTag))
		AbilitySystemComponent->AddLooseGameplayTag(NewEffect.StatusTag);

    // 상태이상 아이콘 패널 활성화
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

    // 역순 순회 (RemoveAt 시 안전)
	for (int32 i = ActiveStatusEffects.Num() - 1; i >= 0; --i)
	{
		FStatusEffectInstance& Effect = ActiveStatusEffects[i];

		if (Effect.StatusTag == TAG_Status_Stun)
		{
            // 스턴: 데미지 없이 턴 스킵 표시
			bWasStunned = true;
		}
		else if (Effect.StatusTag == TAG_Status_Burn)
		{
            // 화상: 스탯 기반 데미지 (방어력 무시)
			ApplyStatusTickDamage(FMath::Max(1.f, Effect.MagnitudePerStack));
		}
		else if (Effect.StatusTag == TAG_Status_Poison)
		{
            // 독: 현재 HP 1% + 스탯 기반 데미지 (방어력 무시)
			const float HPPercent = GetHP() * 0.01f;
			ApplyStatusTickDamage(FMath::Max(1.f, HPPercent + Effect.MagnitudePerStack));
		}
		else if (Effect.StatusTag == TAG_Status_Regen)
		{
            // 재생: 힐
			ApplyStatusTickHeal(FMath::Max(1.f, Effect.MagnitudePerStack));
		}

        // 스택 1 소모
		Effect.RemainingTurns--;
        bChanged = true; // 스택 변경 → 항상 UI 갱신
		if (Effect.RemainingTurns <= 0)
			ActiveStatusEffects.RemoveAt(i);
	}

	SyncStatusTags();

    // 모든 효과가 소진되면 아이콘 패널 숨김
	if (ActiveStatusEffects.IsEmpty() && StatusIconComponent)
		StatusIconComponent->SetVisibility(false);

	if (bChanged || bWasStunned)
		OnStatusEffectsChanged.Broadcast(this);

	return bWasStunned;
}

void ABattleCombatant::SyncStatusTags()
{
	if (!AbilitySystemComponent) return;

    // 현재 ActiveStatusEffects에 있는 태그 목록 수집
	TSet<FGameplayTag> WantedTags;
	for (const FStatusEffectInstance& E : ActiveStatusEffects)
		WantedTags.Add(E.StatusTag);

    // 관리 대상 Status 태그들
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

void ABattleCombatant::RegisterArtifactEffectHandle(FActiveGameplayEffectHandle Handle,
	const FGameplayTagContainer& Traits)
{
	if (!Handle.IsValid())
	{
		return;
	}
	
	FAppliedArtifactEffect Entry;
	Entry.Handle = Handle;
	Entry.Traits = Traits;

	AppliedArtifactEffects.Add(Entry);
}


void ABattleCombatant::ApplyStatusTickDamage(float Damage)
{
	if (!AbilitySystemComponent || Damage <= 0.f) return;

    // 동적 GE로 IncomingDamage 메타 어트리뷰트에 직접 주입
    // → PostGameplayEffectExecute에서 HP 차감 + 데미지 숫자 UI + 사망 처리 자동 연결
	UGameplayEffect* GEObj = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None);
	GEObj->DurationPolicy = EGameplayEffectDurationType::Instant;

    // IncomingDamage 설정
	FGameplayModifierInfo DamageMod;
	DamageMod.Attribute        = UTBAttributeSet::GetIncomingDamageAttribute();
	DamageMod.ModifierOp       = EGameplayModOp::Additive;
	DamageMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(Damage);
	GEObj->Modifiers.Add(DamageMod);

    // IncomingCritical도 설정 (0 = 비크리티컬) - PostGameplayEffectExecute 트리거용
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

// ─── 레벨 스탯 적용 ────────────────────────────────────────────────────────────

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

    // 모든 스탯 직접 적용 (적 캐릭터용 - 현재값 = 최대값)
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

// ─── ATB 게이지 시스템 ────────────────────────────────────────────────────────

void ABattleCombatant::ChargeActionGauge()
{
	ActionGauge += GetSpeed();
}

void ABattleCombatant::ConsumeActionGauge()
{
	ActionGauge -= ActionGaugeThreshold;
    // 음수 방지 (혹시 100 미만에서 행동했을 경우)
	if (ActionGauge < 0.f)
		ActionGauge = 0.f;
}

// ─── 적 스펙 강화 ─────────────────────────────────────────────────────────────
void ABattleCombatant::ApplyEnemyFloorMultiplier(float Multiplier)
{
	// 약화는 고려하지 않음
	if (!AttributeSet || Multiplier <= 1.0f)
	{
		return;
	}

	// StartingEffects 적용이 끝난 뒤의 값을 변경
	const float OldMaxHP = AttributeSet->GetMaxHP();
	const float OldHP = AttributeSet->GetHP();

	const float OldMaxMP = AttributeSet->GetMaxMP();
	const float OldMP = AttributeSet->GetMP();

	const float OldMaxStamina = AttributeSet->GetMaxStamina();
	const float OldStamina = AttributeSet->GetStamina();

	const float OldPhysicalAttack = AttributeSet->GetPhysicalAttack();
	const float OldMagicAttack = AttributeSet->GetMagicAttack();
	const float OldPhysicalDefense = AttributeSet->GetPhysicalDefense();
	const float OldMagicDefense = AttributeSet->GetMagicDefense();
	const float OldCriticalMultiplier = AttributeSet->GetCriticalMultiplier();

	// Max와 Current를 같이 올려서 비율을 유지한다.
	// 예: HP가 80/100이면 1.2배 후 96/120이 되도록 처리.
	const float NewMaxHP = OldMaxHP * Multiplier;
	const float NewHP = FMath::Min(OldHP * Multiplier, NewMaxHP);

	const float NewMaxMP = OldMaxMP * Multiplier;
	const float NewMP = FMath::Min(OldMP * Multiplier, NewMaxMP);

	const float NewMaxStamina = OldMaxStamina * Multiplier;
	const float NewStamina = FMath::Min(OldStamina * Multiplier, NewMaxStamina);

	AttributeSet->SetMaxHP(NewMaxHP);
	AttributeSet->SetHP(NewHP);

	AttributeSet->SetMaxMP(NewMaxMP);
	AttributeSet->SetMP(NewMP);

	AttributeSet->SetMaxStamina(NewMaxStamina);
	AttributeSet->SetStamina(NewStamina);

	// 전투 스탯 강화
	AttributeSet->SetPhysicalAttack(FMath::Max(0.0f, OldPhysicalAttack * Multiplier));
	AttributeSet->SetMagicAttack(FMath::Max(0.0f, OldMagicAttack * Multiplier));
	AttributeSet->SetPhysicalDefense(FMath::Max(0.0f, OldPhysicalDefense * Multiplier));
	AttributeSet->SetMagicDefense(FMath::Max(0.0f, OldMagicDefense * Multiplier));

	// CriticalChance는 제외, CriticalMultiplier는 포함
	AttributeSet->SetCriticalMultiplier(FMath::Max(1.0f, OldCriticalMultiplier * Multiplier));

	// Speed도 제외

	// 체력바 / 상태 UI 갱신용 브로드캐스트
	OnStatChangedInternal();
}






