#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Battle/BattleManager.h"
#include "TBBattleHUD.generated.h"

class UTurnOrderWidget;
class UBattleMenuWidget;
class UCharacterStatusWidget;

/**
 * 배틀 씬 HUD.
 * 위젯 생성/관리 및 BattleManager 델리게이트 구독 담당.
 * Blueprint에서 위젯 클래스를 지정한다.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTTB_API ATBBattleHUD : public AHUD
{
	GENERATED_BODY()

public:
	ATBBattleHUD();

	virtual void BeginPlay() override;
	
	// ─── 전투 배틀 위젯 ─────────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category="Widgets")
	void EnterBattleMode();
	
	UFUNCTION(BlueprintCallable, Category="Widgets")
	void ExitBattleMode();

	// ─── 위젯 클래스 (Blueprint에서 지정) ────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, Category="Widgets")
	TSubclassOf<UTurnOrderWidget> TurnOrderWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Widgets")
	TSubclassOf<UBattleMenuWidget> BattleMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Widgets")
	TSubclassOf<UCharacterStatusWidget> CharacterStatusWidgetClass;

	// ─── 생성된 위젯 참조 ────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category="Widgets")
	TObjectPtr<UTurnOrderWidget> TurnOrderWidget;

	UPROPERTY(BlueprintReadOnly, Category="Widgets")
	TObjectPtr<UBattleMenuWidget> BattleMenuWidget;

	UPROPERTY(BlueprintReadOnly, Category="Widgets")
	TObjectPtr<UCharacterStatusWidget> CharacterStatusWidget;

	// BattleManager 참조
	UPROPERTY(BlueprintReadOnly, Category="Battle")
	TObjectPtr<ABattleManager> BattleManager;

protected:
	void CreateBattleWidgets();
	void RemoveBattleWidgets();
	void BindToBattleManager();
	void UnBindToBattleManager();

	// BattleManager 델리게이트 핸들러
	UFUNCTION()
	void HandlePhaseChanged(EBattlePhase NewPhase);

	UFUNCTION()
	void HandleTurnBegin(ABattleCombatant* Combatant);

	UFUNCTION()
	void HandleTurnOrderUpdated(const TArray<ABattleCombatant*>& UpcomingTurns);

	UFUNCTION()
	void HandleDamageDealt(ABattleCombatant* Target, float Damage);

	UFUNCTION()
	void HandleHealDealt(ABattleCombatant* Target, float Heal);

	UFUNCTION()
	void HandleStatChanged(ABattleCombatant* Combatant);

	UFUNCTION()
	void HandleBattleReady();
};
