#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleCombatant.h"
#include "Abilities/TBGameplayAbility.h"
#include "BattleEnemyCharacter.generated.h"

class UWidgetComponent;
class UEnemyHealthBarWidget;

// ─── 타겟 선택 조건 ────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class ETargetCondition : uint8
{
	Random,          // 랜덤
	LowestHP,        // HP 절댓값이 가장 낮은 대상
	HighestHP,       // HP 절댓값이 가장 높은 대상
	LowestHPPercent, // HP 비율이 가장 낮은 대상
	HighestHPPercent,// HP 비율이 가장 높은 대상
	Self             // 자기 자신 (버프/힐용)
};

// ─── 스킬 항목 (설정 + 런타임 추적) ──────────────────────────────────────────
USTRUCT(BlueprintType)
struct FEnemySkillEntry
{
	GENERATED_BODY()

	// 사용할 어빌리티
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UTBGameplayAbility> AbilityClass;

	// 자신 HP 조건 (비율, 0~1)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin=0, ClampMax=1))
	float MinSelfHPPercent = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin=0, ClampMax=1))
	float MaxSelfHPPercent = 1.f;

	// 우선순위 (높을수록 먼저 평가, 동일 우선순위 내 Weight로 가중치 랜덤)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Priority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin=0.001))
	float Weight = 1.f;

	// 타겟 선택 방식
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	ETargetCondition TargetCondition = ETargetCondition::Random;

	// 사용 후 N턴(이 적의 턴 기준) 동안 재사용 불가 (0 = 쿨다운 없음)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin=0))
	int32 CooldownTurns = 0;

	// 전투 내 최대 사용 횟수 (-1 = 무제한)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 MaxUseCount = -1;

	// ─── 런타임 (에디터 노출 안 함) ────────────────────────────────────────
	int32 LastUsedTurn = -9999;
	int32 UsedCount    = 0;
};

// ─── 페이즈 (일반 적은 bUsePhaseSystem=false로 비활성) ───────────────────────
USTRUCT(BlueprintType)
struct FPhaseData
{
	GENERATED_BODY()

	// 이 HP% 이하로 떨어지면 페이즈 진입 (예: 0.5 = 50% 이하)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin=0, ClampMax=1))
	float HPThreshold = 0.5f;

	// 이 페이즈에서 사용할 스킬 테이블
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FEnemySkillEntry> PhaseSkills;

	// 페이즈 진입 시 즉시 실행할 어빌리티 (없으면 바로 PhaseSkills 사용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UTBGameplayAbility> PhaseTransitionAbility;

	// ─── 런타임 ────────────────────────────────────────────────────────────
	bool bActivated = false;
};

/**
 * 적 캐릭터 기반 클래스.
 * AI 행동은 SelectAction을 Blueprint에서 오버라이드해 구현.
 * 기본 구현: DefaultSkills 테이블 평가 → 조건/우선순위/가중치로 스킬 선택.
 * DefaultSkills가 비어있으면 기존 랜덤 동작 유지 (하위 호환).
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTTB_API ABattleEnemyCharacter : public ABattleCombatant
{
	GENERATED_BODY()

public:
	ABattleEnemyCharacter();

	// Blueprint에서 WBP_EnemyHealthBar 클래스를 지정한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UUserWidget> HealthBarWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="UI")
	TObjectPtr<UWidgetComponent> HealthBarWidgetComponent;

	// 적 크기마다 다를 수 있으므로 Blueprint에서 개별 조정
	UPROPERTY(EditDefaultsOnly, Category="UI", meta=(DisplayName="HP바 Z 위치"))
	float HealthBarZOffset = 120.f;

	UPROPERTY(EditDefaultsOnly, Category="UI", meta=(DisplayName="HP바 크기"))
	FVector2D HealthBarDrawSize = FVector2D(150.f, 15.f);

	UPROPERTY(EditDefaultsOnly, Category="UI", meta=(DisplayName="상태이상 아이콘 Z 위치"))
	float StatusIconZOffset = 140.f; // HP바(120) 위

	// ─── 보상 ─────────────────────────────────────────────────────────────────
	/** 처치 시 파티에게 주는 경험치 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Reward")
	int32 ExpReward = 10;

	// ─── AI 스킬 테이블 ────────────────────────────────────────────────────────
	/** 기본 스킬 테이블. 비어있으면 랜덤 어빌리티 동작(하위 호환). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Skills")
	TArray<FEnemySkillEntry> DefaultSkills;

	// ─── 페이즈 시스템 ────────────────────────────────────────────────────────
	/** true면 Phases 배열 사용, false면 DefaultSkills만 사용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Phase")
	bool bUsePhaseSystem = false;

	/**
	 * 페이즈 목록. HPThreshold 내림차순으로 설정 권장 (높은 % → 낮은 % 순).
	 * 여러 페이즈가 동시에 활성화될 경우 가장 마지막으로 진입한 페이즈를 사용.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Phase",
		meta=(EditCondition="bUsePhaseSystem"))
	TArray<FPhaseData> Phases;

	// ─── 도발 ─────────────────────────────────────────────────────────────────
	/**
	 * 이 적에게 도발 적용.
	 * 상태이상 아이콘(Status.Taunt) 부여 + TauntTarget/TauntRemainingTurns 세팅.
	 * GA_Taunt Blueprint에서 살아있는 적 순회하며 호출.
	 */
	UFUNCTION(BlueprintCallable, Category="AI|Taunt")
	void ApplyTaunt(ABattleCombatant* Caster, int32 Turns);

	/** 도발 시 시전자를 타겟으로 선택할 확률 (0~1, 기본 0.75) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Taunt",
		meta=(ClampMin=0, ClampMax=1))
	float TauntAggroChance = 0.75f;

	/**
	 * AI 행동 선택.
	 * Blueprint에서 오버라이드해 적마다 다른 패턴 구현 가능.
	 * @param PlayerParty  살아있는 플레이어 배열
	 * @param OutTarget    선택한 타겟
	 * @param OutAbilityClass 사용할 어빌리티 클래스
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="AI")
	void SelectAction(
		const TArray<ABattleCombatant*>& PlayerParty,
		ABattleCombatant*& OutTarget,
		TSubclassOf<UTBGameplayAbility>& OutAbilityClass);

	virtual void SelectAction_Implementation(
		const TArray<ABattleCombatant*>& PlayerParty,
		ABattleCombatant*& OutTarget,
		TSubclassOf<UTBGameplayAbility>& OutAbilityClass);

	virtual void OnTurnBegin_Implementation() override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnDamageReceivedHandler(ABattleCombatant* Combatant, float Damage, bool bIsCritical);

	void RefreshHealthBar();

	// ─── AI 내부 ──────────────────────────────────────────────────────────────
	/** 이 적이 행동한 총 턴 수 (쿨다운 계산 기준) */
	int32 CurrentBattleTurn = 0;

	/** 현재 활성 페이즈 인덱스 (-1 = 페이즈 없음 → DefaultSkills 사용) */
	int32 ActivePhaseIndex = -1;

	// ─── 도발 런타임 ──────────────────────────────────────────────────────────
	/** 도발 시전자. 유효하지 않으면 도발 없음 */
	TWeakObjectPtr<ABattleCombatant> TauntTarget;

	/** 도발 남은 턴 수. 0 되면 TauntTarget 클리어 */
	int32 TauntRemainingTurns = 0;

	/**
	 * 조건에 따라 타겟 반환.
	 * Self일 경우 this 반환, 그 외 Living 배열에서 선택.
	 */
	ABattleCombatant* ResolveTarget(ETargetCondition Condition,
	                                const TArray<ABattleCombatant*>& Living);

	/**
	 * 스킬 테이블에서 조건/우선순위/가중치로 스킬 선택.
	 * 반환값: 선택된 항목의 인덱스 (-1 = 없음).
	 */
	int32 SelectSkillFromPool(TArray<FEnemySkillEntry>& Pool, float SelfHPPercent);
};
