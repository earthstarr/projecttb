#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Battle/BattleManager.h"
#include "Data/LevelDataTypes.h"
#include "TBBattleHUD.generated.h"

class AWorldPlayerController;
class UTurnOrderWidget;
class UBattleMenuWidget;
class UCharacterStatusWidget;
class UVictoryWidget;
class ULevelUpWidget;
class UPortalFloorWidget;
class UScreenFadeBlockerWidget;

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

	UFUNCTION(BlueprintCallable, Category="Widgets")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category="Widgets")
	void ShowPortalFloorWidget();

	UFUNCTION(BlueprintCallable, Category="Widgets")
	void RemovePortalFloorWidget();

	UFUNCTION(BlueprintCallable, Category="Widgets")
	void ShowScreenFadeBlocker();

	UFUNCTION(BlueprintCallable, Category="Widgets")
	void HideScreenFadeBlocker();

	UFUNCTION(BlueprintCallable, Category="Widgets")
	void RemoveScreenFadeBlockerImmediately();
	
	
	
	// ─── 맵 이동 페이드 인, 아웃 ─────────────────────────────────────────────
	UPROPERTY()
	APlayerCameraManager* CamManager;
	
	//UPROPERTY()
	//AWorldPlayerController* PC;
	
	float TransitionStartTime;
	
	void StartFadeOut(float Duration = 0.2f);
	void StartFadeIn(float Duration = 0.2f);
	
	// ─── 위젯 클래스 (Blueprint에서 지정) ────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, Category="Widgets")
	TSubclassOf<UTurnOrderWidget> TurnOrderWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Widgets")
	TSubclassOf<UBattleMenuWidget> BattleMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Widgets")
	TSubclassOf<UCharacterStatusWidget> CharacterStatusWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Widgets")
	TSubclassOf<UVictoryWidget> VictoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Widgets")
	TSubclassOf<ULevelUpWidget> LevelUpWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Widgets")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Widgets")
	TSubclassOf<UUserWidget> DefeatWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Widgets")
	TSubclassOf<UPortalFloorWidget> PortalFloorWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Widgets")
	TSubclassOf<UScreenFadeBlockerWidget> ScreenFadeBlockerWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category="Widgets")
	TSubclassOf<UUserWidget> FinalVictoryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Widgets")
	TSubclassOf<UUserWidget> FinalVictorySecondWidgetClass;

	// ─── 생성된 위젯 참조 ────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category="Widgets")
	TObjectPtr<UTurnOrderWidget> TurnOrderWidget;

	UPROPERTY(BlueprintReadOnly, Category="Widgets")
	TObjectPtr<UBattleMenuWidget> BattleMenuWidget;

	UPROPERTY(BlueprintReadOnly, Category="Widgets")
	TObjectPtr<UCharacterStatusWidget> CharacterStatusWidget;

	UPROPERTY(BlueprintReadOnly, Category="Widgets")
	TObjectPtr<UVictoryWidget> VictoryWidget;

	UPROPERTY(BlueprintReadOnly, Category="Widgets")
	TObjectPtr<ULevelUpWidget> LevelUpWidget;

	UPROPERTY(BlueprintReadOnly, Category="Widgets")
	TObjectPtr<UUserWidget> MainMenuWidget;

	UPROPERTY(BlueprintReadOnly, Category="Widgets")
	TObjectPtr<UUserWidget> DefeatWidget;

	UPROPERTY(BlueprintReadOnly, Category="Widgets")
	TObjectPtr<UPortalFloorWidget> PortalFloorWidget;

	UPROPERTY(BlueprintReadOnly, Category="Widgets")
	TObjectPtr<UScreenFadeBlockerWidget> ScreenFadeBlockerWidget;

	UPROPERTY(BlueprintReadOnly, Category="Widgets")
	TObjectPtr<UUserWidget> FinalVictoryWidget;

	UPROPERTY(BlueprintReadOnly, Category="Widgets")
	TObjectPtr<UUserWidget> FinalVictorySecondWidget;
	// ─── Victory/LevelUp 함수 ────────────────────────────────────────────────

	/** 승리 위젯 표시 */
	UFUNCTION(BlueprintCallable, Category="Victory")
	void ShowVictoryWidget(const TArray<FCharacterExpData>& BeforeExpData,
	                       const TArray<FCharacterExpData>& AfterExpData,
	                       const TArray<FLevelUpDisplayData>& LevelUpData,
	                       int32 RewardMoney);

	/** 레벨업 위젯 표시 */
	UFUNCTION(BlueprintCallable, Category="Victory")
	void ShowLevelUpWidget(const TArray<FLevelUpDisplayData>& LevelUpData);

	/** 월드로 복귀 */
	UFUNCTION(BlueprintCallable, Category="Victory")
	void ReturnToWorld();

	/** 패배 위젯 표시 */
	UFUNCTION(BlueprintCallable, Category="Defeat")
	void ShowDefeatWidget();

	/** 최종 승리 위젯 표시 */
	UFUNCTION(BlueprintCallable, Category="Victory")
	void ShowFinalVictoryWidget();

	/** 최종 승리 2단계 위젯 표시 (크레딧 등) */
	UFUNCTION(BlueprintCallable, Category="Victory")
	void ShowFinalVictorySecondWidget();

	/** 메인 메뉴로 복귀 */
	UFUNCTION(BlueprintCallable, Category="Defeat")
	void ReturnToMainMenu();

	// BattleManager 참조
	UFUNCTION(BlueprintCallable, Category="Battle")
	void SetBattleManager(ABattleManager* NewBattleManager);

	UPROPERTY(BlueprintReadOnly, Category="Battle")
	TObjectPtr<ABattleManager> BattleManager;

	void RemoveBattleWidgets();

protected:
	void CreateBattleWidgets();
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

	UFUNCTION()
	void HandleDiceRolled(int32 FaceValue, float Multiplier);

	// Blueprint에서 오버레이 위젯 표시/숨김 구현
	UFUNCTION(BlueprintImplementableEvent, Category="Dice")
	void ShowDiceResultOverlay(int32 FaceValue, float Multiplier);

	UFUNCTION(BlueprintImplementableEvent, Category="Dice")
	void HideDiceResultOverlay();

private:
	FTimerHandle DiceOverlayTimer;
};
