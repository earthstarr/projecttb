#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Abilities/TBGameplayAbility.h"
#include "BattleManager.generated.h"

class ABattleCombatant;
class ABattlePlayerCharacter;
class ABattleEnemyCharacter;
class UTBGameplayAbility;
class ACameraActor;

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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAnyHealDealt,      ABattleCombatant*, Target, float, Heal);

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

	// 방어 선택 — 이번 턴 받는 데미지 50% 감소, 즉시 턴 종료
	UFUNCTION(BlueprintCallable, Category="Battle|Input")
	void PlayerSelectDefend();

	// 취소 (메뉴로 복귀)
	UFUNCTION(BlueprintCallable, Category="Battle|Input")
	void PlayerCancel();

	// ─── 어빌리티 완료 콜백 (Ability → BattleManager) ────────────────────────
	UFUNCTION(BlueprintCallable, Category="Battle")
	void OnActionComplete();

	// ─── 쿼리 ────────────────────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category="Battle")
	ABattleCombatant* GetCurrentActor() const;

	// TurnOrderWidget에 전달할 다음 N턴 목록 (ATB 게이지 시뮬레이션 기반)
	UFUNCTION(BlueprintCallable, Category="Battle")
	TArray<ABattleCombatant*> GetUpcomingTurns(int32 Count = 10) const;

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

	// 현재 선택 중인 타겟 타입 (UI에서 사용)
	UFUNCTION(BlueprintCallable, Category="Battle")
	EAbilityTargetType GetPendingTargetType() const { return PendingTargetType; }

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

	// ─── 카메라 ───────────────────────────────────────────────────────────────
	// 전투 전체 고정 카메라 (레벨에 배치)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	TObjectPtr<ACameraActor> BattleCamera;

	// 공격/스킬 실행 시 사용하는 카메라 (레벨에 하나 배치, BattleManager가 위치 제어)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	TObjectPtr<ACameraActor> ActionCamera;

	// 플레이어 턴 메뉴 시 캐릭터 옆 3인칭 카메라 오프셋 (캐릭터 로컬 기준)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	FVector PlayerTurnCameraOffset = FVector(-150.f, 80.f, 100.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	FRotator PlayerTurnCameraRotation = FRotator(-15.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	float PlayerTurnCameraBlendTime = 0.5f;

	// ─── 이벤트 델리게이트 ───────────────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnBattlePhaseChanged OnBattlePhaseChanged;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnTurnBegin OnTurnBegin;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnTurnOrderUpdated OnTurnOrderUpdated;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnAnyDamageDealt OnAnyDamageDealt;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnAnyHealDealt OnAnyHealDealt;

protected:
	virtual void BeginPlay() override;

private:
	// 상태
	EBattlePhase CurrentPhase = EBattlePhase::None;

	// 파티
	TArray<ABattlePlayerCharacter*> PlayerParty;
	TArray<ABattleEnemyCharacter*>  EnemyParty;

	// ATB 시스템: 현재 행동 중인 캐릭터
	ABattleCombatant* CurrentActor = nullptr;

	// 진행 중인 액션 데이터
	ABattleCombatant*               PendingTarget       = nullptr;
	TSubclassOf<UTBGameplayAbility> PendingAbilityClass = nullptr;
	EAbilityTargetType              PendingTargetType   = EAbilityTargetType::SingleEnemy;

	FTimerHandle EnemyActionTimer;

	// 카메라 상태
	bool  bActionCameraActive       = false;
	float PendingCameraBlendOutTime = 0.5f;

	// 내부 메서드
	void SwitchToActionCamera(ABattleCombatant* Caster, const UTBGameplayAbility* AbilityCDO);
	void SwitchToPlayerTurnCamera();
	void ReturnToBattleCamera();

public:
	// 컷씬/어빌리티에서 카메라 위치를 동적으로 변경 (월드 좌표)
	UFUNCTION(BlueprintCallable, Category="Battle|Camera")
	void SetActionCameraWorldPosition(const FVector& WorldLocation, const FRotator& WorldRotation, float BlendTime = 0.3f);

	// 액션 카메라 활성화 상태 반환
	UFUNCTION(BlueprintCallable, Category="Battle|Camera")
	bool IsActionCameraActive() const { return bActionCameraActive; }

	// ActionCamera 접근자
	UFUNCTION(BlueprintCallable, Category="Battle|Camera")
	ACameraActor* GetActionCamera() const { return ActionCamera; }

private:

	void SetPhase(EBattlePhase NewPhase);
	void AdvanceTurn();

	// ATB 시스템: 모든 캐릭터 게이지 충전 후 행동 가능한 캐릭터 반환
	ABattleCombatant* ChargeAndFindNextActor();

	// ATB 시스템: 게이지 시뮬레이션으로 다음 N턴 예측
	TArray<ABattleCombatant*> SimulateUpcomingTurns(int32 Count) const;
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
	void OnCombatantDamaged(ABattleCombatant* Combatant, float Damage, bool bIsCritical);

	UFUNCTION()
	void OnCombatantHealed(ABattleCombatant* Combatant, float Heal);

	// bAutoStartBattle 전용 — 딜레이 후 호출
	void AutoStartBattle();

	// 셰이더 웜업 — StartBattle 직후 오프스크린에서 이펙트 미리 터뜨림
	void WarmUpEffects();

	// 스턴 턴 스킵 딜레이용 (0.75초 표시 후 OnActionComplete)
	FTimerHandle StunSkipTimer;


	// 전투 종료 시 경험치 분배 및 스탯 저장
	void HandleBattleVictory();
	void HandleBattleDefeat();
	void SavePartyStats();
};
