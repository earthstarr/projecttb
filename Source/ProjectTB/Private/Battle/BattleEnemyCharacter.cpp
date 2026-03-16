#include "Battle/BattleEnemyCharacter.h"
#include "Abilities/TBGameplayAbility.h"
#include "Components/WidgetComponent.h"
#include "UI/EnemyHealthBarWidget.h"
#include "TBGameplayTags.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

ABattleEnemyCharacter::ABattleEnemyCharacter()
{
	HealthBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
	HealthBarWidgetComponent->SetupAttachment(GetRootComponent());
	HealthBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);

	// 상태이상 아이콘: 우하단 기준 → 오른쪽 고정, 왼쪽으로 쌓임 (위치는 BeginPlay에서 변수로 적용)
	StatusIconComponent->SetPivot(FVector2D(1.0f, 1.0f));
}

void ABattleEnemyCharacter::BeginPlay()
{
	Super::BeginPlay(); // BattleCombatant::BeginPlay → InitAbilitySystem (스탯 세팅 완료)

	// Blueprint 변수로 위치/크기 적용
	HealthBarWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, HealthBarZOffset));
	HealthBarWidgetComponent->SetDrawSize(HealthBarDrawSize);
	StatusIconComponent->SetRelativeLocation(FVector(0.f, 0.f, StatusIconZOffset));

	if (HealthBarWidgetClass)
		HealthBarWidgetComponent->SetWidgetClass(HealthBarWidgetClass);

	// 체력바 위젯에 Combatant 전달 (내부 StatusIconPanel 초기화용)
	if (UEnemyHealthBarWidget* Widget = Cast<UEnemyHealthBarWidget>(HealthBarWidgetComponent->GetUserWidgetObject()))
		Widget->InitWithCombatant(this);

	// 초기 체력바 표시
	RefreshHealthBar();

	// 데미지 받을 때마다 체력바 갱신
	OnDamageReceived.AddDynamic(this, &ABattleEnemyCharacter::OnDamageReceivedHandler);
}

void ABattleEnemyCharacter::OnDamageReceivedHandler(ABattleCombatant* /*Combatant*/, float /*Damage*/, bool /*bIsCritical*/)
{
	RefreshHealthBar();
}

void ABattleEnemyCharacter::RefreshHealthBar()
{
	if (UEnemyHealthBarWidget* Widget = Cast<UEnemyHealthBarWidget>(HealthBarWidgetComponent->GetUserWidgetObject()))
		Widget->UpdateHealth(GetHP(), GetMaxHP());
}

void ABattleEnemyCharacter::ApplyTaunt(ABattleCombatant* Caster, int32 Turns)
{
	if (!Caster || Turns <= 0) return;

	TauntTarget = Caster;
	TauntRemainingTurns = Turns;

	// 아이콘 표시용 상태이상 적용
	FStatusEffectInstance TauntEffect;
	TauntEffect.StatusTag = TAG_Status_Taunt;
	TauntEffect.MagnitudePerStack = 0.f;
	TauntEffect.RemainingTurns = Turns;
	ApplyStatusEffect(TauntEffect);
}

void ABattleEnemyCharacter::SelectAction_Implementation(
	const TArray<ABattleCombatant*>& PlayerParty,
	ABattleCombatant*& OutTarget,
	TSubclassOf<UTBGameplayAbility>& OutAbilityClass)
{
	TArray<ABattleCombatant*> Living = PlayerParty.FilterByPredicate(
		[](const ABattleCombatant* C) { return C && !C->IsDead(); });

	if (Living.IsEmpty()) return;

	// DefaultSkills가 비어있고 페이즈 시스템도 꺼져있으면 기존 랜덤 동작 (하위 호환)
	if (DefaultSkills.IsEmpty() && !bUsePhaseSystem)
	{
		OutTarget = Living[FMath::RandRange(0, Living.Num() - 1)];
		TArray<UTBGameplayAbility*> Abilities = GetGrantedAbilities();
		if (!Abilities.IsEmpty())
			OutAbilityClass = Abilities[FMath::RandRange(0, Abilities.Num() - 1)]->GetClass();
		return;
	}

	const float SelfHPPercent = GetMaxHP() > 0.f ? GetHP() / GetMaxHP() : 0.f;

	// ─── 페이즈 체크 ─────────────────────────────────────────────────────────
	if (bUsePhaseSystem)
	{
		for (int32 i = 0; i < Phases.Num(); i++)
		{
			FPhaseData& Phase = Phases[i];
			if (!Phase.bActivated && SelfHPPercent <= Phase.HPThreshold)
			{
				Phase.bActivated = true;
				ActivePhaseIndex = i;

				// 페이즈 이펙트 활성화
				ActivatePhaseEffect(i);

				// 페이즈 전환 어빌리티가 있으면 이번 턴에 즉시 사용
				if (Phase.PhaseTransitionAbility)
				{
					OutAbilityClass = Phase.PhaseTransitionAbility;
					OutTarget = Living[FMath::RandRange(0, Living.Num() - 1)];
					return;
				}
			}
			else if (Phase.bActivated)
			{
				// 이미 활성된 페이즈 중 가장 낮은(강한) 페이즈 추적
				ActivePhaseIndex = i;
			}
		}
	}

	// ─── 활성 스킬 풀 결정 ───────────────────────────────────────────────────
	TArray<FEnemySkillEntry>* ActivePool = &DefaultSkills;
	if (bUsePhaseSystem && ActivePhaseIndex >= 0 && Phases.IsValidIndex(ActivePhaseIndex))
		ActivePool = &Phases[ActivePhaseIndex].PhaseSkills;

	// ─── 조건/우선순위/가중치로 스킬 선택 ────────────────────────────────────
	const int32 SelectedIndex = SelectSkillFromPool(*ActivePool, SelfHPPercent);

	if (SelectedIndex >= 0)
	{
		FEnemySkillEntry& Entry = (*ActivePool)[SelectedIndex];
		Entry.LastUsedTurn = CurrentBattleTurn;
		Entry.UsedCount++;

		OutAbilityClass = Entry.AbilityClass;

		// 도발 체크: SingleEnemy 스킬일 때만, TauntAggroChance 확률로 시전자 우선 타겟
		// AllEnemies/Self 등은 기존 ResolveTarget 그대로 사용
		const UTBGameplayAbility* CDO = Entry.AbilityClass
			? Entry.AbilityClass->GetDefaultObject<UTBGameplayAbility>()
			: nullptr;

		const bool bSingleEnemy = CDO && CDO->TargetType == EAbilityTargetType::SingleEnemy;
		const bool bTauntValid  = TauntTarget.IsValid() && !TauntTarget->IsDead();

		if (bSingleEnemy && bTauntValid && FMath::FRandRange(0.f, 1.f) < TauntAggroChance)
			OutTarget = TauntTarget.Get();
		else
			OutTarget = ResolveTarget(Entry.TargetCondition, Living);
	}
	else
	{
		// 폴백: 조건 맞는 스킬 없음 → 첫 번째 어빌리티 + 랜덤 타겟
		TArray<UTBGameplayAbility*> Abilities = GetGrantedAbilities();
		if (!Abilities.IsEmpty())
			OutAbilityClass = Abilities[0]->GetClass();
		OutTarget = Living[FMath::RandRange(0, Living.Num() - 1)];
	}
}

int32 ABattleEnemyCharacter::SelectSkillFromPool(TArray<FEnemySkillEntry>& Pool, float SelfHPPercent)
{
	// 1. 조건 필터링
	TArray<int32> ValidIndices;
	for (int32 i = 0; i < Pool.Num(); i++)
	{
		const FEnemySkillEntry& Entry = Pool[i];
		if (!Entry.AbilityClass) continue;
		if (SelfHPPercent < Entry.MinSelfHPPercent || SelfHPPercent > Entry.MaxSelfHPPercent) continue;
		if (Entry.CooldownTurns > 0 && (CurrentBattleTurn - Entry.LastUsedTurn) <= Entry.CooldownTurns) continue;
		if (Entry.MaxUseCount >= 0 && Entry.UsedCount >= Entry.MaxUseCount) continue;
		ValidIndices.Add(i);
	}

	if (ValidIndices.IsEmpty()) return -1;

	// 2. 최고 우선순위 그룹 추출
	int32 MaxPriority = TNumericLimits<int32>::Min();
	for (int32 i : ValidIndices)
		MaxPriority = FMath::Max(MaxPriority, Pool[i].Priority);

	TArray<int32> TopIndices;
	for (int32 i : ValidIndices)
		if (Pool[i].Priority == MaxPriority)
			TopIndices.Add(i);

	// 3. 가중치 랜덤 선택
	float TotalWeight = 0.f;
	for (int32 i : TopIndices)
		TotalWeight += Pool[i].Weight;

	const float Rand = FMath::FRandRange(0.f, TotalWeight);
	float Cumulative = 0.f;
	for (int32 i : TopIndices)
	{
		Cumulative += Pool[i].Weight;
		if (Rand <= Cumulative)
			return i;
	}

	return TopIndices.Last();
}

ABattleCombatant* ABattleEnemyCharacter::ResolveTarget(ETargetCondition Condition,
                                                        const TArray<ABattleCombatant*>& Living)
{
	if (Living.IsEmpty()) return nullptr;

	switch (Condition)
	{
	case ETargetCondition::LowestHP:
	{
		ABattleCombatant* Best = Living[0];
		for (ABattleCombatant* C : Living)
			if (C->GetHP() < Best->GetHP()) Best = C;
		return Best;
	}
	case ETargetCondition::HighestHP:
	{
		ABattleCombatant* Best = Living[0];
		for (ABattleCombatant* C : Living)
			if (C->GetHP() > Best->GetHP()) Best = C;
		return Best;
	}
	case ETargetCondition::LowestHPPercent:
	{
		ABattleCombatant* Best = Living[0];
		float BestPct = Best->GetMaxHP() > 0.f ? Best->GetHP() / Best->GetMaxHP() : 0.f;
		for (ABattleCombatant* C : Living)
		{
			const float Pct = C->GetMaxHP() > 0.f ? C->GetHP() / C->GetMaxHP() : 0.f;
			if (Pct < BestPct) { Best = C; BestPct = Pct; }
		}
		return Best;
	}
	case ETargetCondition::HighestHPPercent:
	{
		ABattleCombatant* Best = Living[0];
		float BestPct = Best->GetMaxHP() > 0.f ? Best->GetHP() / Best->GetMaxHP() : 1.f;
		for (ABattleCombatant* C : Living)
		{
			const float Pct = C->GetMaxHP() > 0.f ? C->GetHP() / C->GetMaxHP() : 0.f;
			if (Pct > BestPct) { Best = C; BestPct = Pct; }
		}
		return Best;
	}
	case ETargetCondition::Self:
		return this;
	case ETargetCondition::Random:
	default:
		return Living[FMath::RandRange(0, Living.Num() - 1)];
	}
}

void ABattleEnemyCharacter::OnTurnBegin_Implementation()
{
	Super::OnTurnBegin_Implementation();
	++CurrentBattleTurn; // 쿨다운 계산용 턴 카운터

	// 도발 턴 감소 → 0이 되면 해제
	if (TauntRemainingTurns > 0)
	{
		--TauntRemainingTurns;
		if (TauntRemainingTurns == 0)
			TauntTarget = nullptr;
	}
}

void ABattleEnemyCharacter::ActivatePhaseEffect(int32 PhaseIndex)
{
	if (!Phases.IsValidIndex(PhaseIndex)) return;

	const FPhaseData& Phase = Phases[PhaseIndex];

	// 이전 Phase 이펙트 제거
	if (ActivePhaseNiagaraComponent)
	{
		ActivePhaseNiagaraComponent->DestroyComponent();
		ActivePhaseNiagaraComponent = nullptr;
	}

	// 새 Phase 이펙트 활성화
	if (Phase.PhaseNiagaraEffect)
	{
		// 캐릭터에 Attach된 Niagara 컴포넌트 생성
		ActivePhaseNiagaraComponent = NewObject<UNiagaraComponent>(this, UNiagaraComponent::StaticClass());
		if (ActivePhaseNiagaraComponent)
		{
			ActivePhaseNiagaraComponent->SetAsset(Phase.PhaseNiagaraEffect);
			ActivePhaseNiagaraComponent->SetupAttachment(GetRootComponent());
			ActivePhaseNiagaraComponent->SetRelativeLocation(Phase.EffectOffset);
			ActivePhaseNiagaraComponent->RegisterComponent();
			ActivePhaseNiagaraComponent->Activate(true);
		}
	}
}
