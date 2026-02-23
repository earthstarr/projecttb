#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleManager.generated.h"

class ABattleCombatant;
class ABattlePlayerCharacter;
class ABattleEnemyCharacter;
class UTBGameplayAbility;

UENUM(BlueprintType)
enum class EBattlePhase : uint8
{
	None,
	BattleStart,      // 전투 시작 연출
	PlayerTurn,       // 플레이어 메뉴 입력 대기
	SelectingTarget,  // 타겟 선택 대기
	ExecutingAction,  // 어빌리티 애니메이션 재생 중
	EnemyTurn,        // 적 AI 행동 처리 중
	BattleVictory,
	BattleDefeat
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBattlePhaseChanged, EBattlePhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnBegin,          ABattleCombatant*, Combatant);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnOrderUpdated,   const TArray<ABattleCombatant*>&, UpcomingTurns);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAnyDamageDealt,    ABattleCombatant*, Target, float, Damage);

/**
 * 배틀 씬에 배치되는 전투 관리 Actor.
 * 턴 순서, 상태머신, 액션 실행을 담당.
 * BattleLevel에 하나 배치하고 GameMode에서 참조한다.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTTB_API ABattleManager : public AActor
{
	GENERATED_BODY()

public:
	ABattleManager();

	// ─── 전투 시작 ───────────────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category="Battle")
	void StartBattle(
		const TArray<ABattlePlayerCharacter*>& Players,
		const TArray<ABattleEnemyCharacter*>&  Enemies);

	// ─── 플레이어 입력 (BattleMenuWidget → BattleManager) ────────────────────
	UFUNCTION(BlueprintCallable, Category="Battle|Input")
	void PlayerSelectAttack();

	UFUNCTION(BlueprintCallable, Category="Battle|Input")
	void PlayerSelectAbility(TSubclassOf<UTBGameplayAbility> AbilityClass);

	// 타겟 선택 완료
	UFUNCTION(BlueprintCallable, Category="Battle|Input")
	void PlayerSelectTarget(ABattleCombatant* Target);

	// 취소 (메뉴로 복귀)
	UFUNCTION(BlueprintCallable, Category="Battle|Input")
	void PlayerCancel();

	// ─── 어빌리티 완료 콜백 (Ability → BattleManager) ────────────────────────
	UFUNCTION(BlueprintCallable, Category="Battle")
	void OnActionComplete();

	// ─── 쿼리 ────────────────────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category="Battle")
	ABattleCombatant* GetCurrentActor() const;

	// TurnOrderWidget에 전달할 다음 N턴 목록
	UFUNCTION(BlueprintCallable, Category="Battle")
	TArray<ABattleCombatant*> GetUpcomingTurns(int32 Count = 5) const;

	UFUNCTION(BlueprintCallable, Category="Battle")
	TArray<ABattleCombatant*> GetLivingPlayers() const;

	UFUNCTION(BlueprintCallable, Category="Battle")
	TArray<ABattleCombatant*> GetLivingEnemies() const;

	// CharacterStatusWidget 초기화용 — 전체 파티 (사망 포함)
	UFUNCTION(BlueprintCallable, Category="Battle")
	TArray<ABattlePlayerCharacter*> GetPlayerParty() const;

	UFUNCTION(BlueprintCallable, Category="Battle")
	EBattlePhase GetCurrentPhase() const { return CurrentPhase; }

	// 어빌리티에서 타겟을 읽어갈 때 사용
	UFUNCTION(BlueprintCallable, Category="Battle")
	ABattleCombatant* GetPendingTarget() const { return PendingTarget; }

	// ─── 테스트용 자동 시작 ───────────────────────────────────────────────────
	// 레벨에 배치된 모든 Player/EnemyCharacter를 자동 감지해서 전투 시작
	// GameInstance 방식 구현 후 false로 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Battle|Debug")
	bool bAutoStartBattle = true;

	// ─── 캐릭터 스폰 위치 (레벨에서 설정) ────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	TArray<TObjectPtr<AActor>> PlayerSpawnPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
	TArray<TObjectPtr<AActor>> EnemySpawnPoints;

	// ─── 이벤트 델리게이트 ───────────────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnBattlePhaseChanged OnBattlePhaseChanged;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnTurnBegin OnTurnBegin;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnTurnOrderUpdated OnTurnOrderUpdated;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnAnyDamageDealt OnAnyDamageDealt;

protected:
	virtual void BeginPlay() override;

private:
	// 상태
	EBattlePhase CurrentPhase = EBattlePhase::None;

	// 파티
	TArray<ABattlePlayerCharacter*> PlayerParty;
	TArray<ABattleEnemyCharacter*>  EnemyParty;

	// 턴 순서 (Speed 기준 내림차순, 라운드 반복)
	TArray<ABattleCombatant*> CurrentRoundOrder;
	int32 CurrentRoundIndex = 0;

	// 진행 중인 액션 데이터
	ABattleCombatant*              PendingTarget      = nullptr;
	TSubclassOf<UTBGameplayAbility> PendingAbilityClass = nullptr;

	FTimerHandle EnemyActionTimer;

	// 내부 메서드
	void SetPhase(EBattlePhase NewPhase);
	void BuildRoundOrder();
	void AdvanceTurn();
	void StartPlayerTurn();
	void StartEnemyTurn();
	void ExecuteEnemyActionDelayed();
	void ExecuteAction(ABattleCombatant* Caster, ABattleCombatant* Target,
	                   TSubclassOf<UTBGameplayAbility> AbilityClass);
	void CheckBattleEnd();
	void BroadcastTurnOrder();

	UFUNCTION()
	void OnCombatantDied(ABattleCombatant* Combatant);

	UFUNCTION()
	void OnCombatantDamaged(ABattleCombatant* Combatant, float Damage);

	// bAutoStartBattle 전용 — 딜레이 후 호출
	void AutoStartBattle();
};
