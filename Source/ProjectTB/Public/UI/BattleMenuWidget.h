#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Battle/TBDiceData.h"
#include "BattleMenuWidget.generated.h"

class ABattleManager;
class ABattleCombatant;
class UTBGameplayAbility;

UENUM(BlueprintType)
enum class EMenuState : uint8
{
	Hidden,
	MainMenu,          // Attack / Abilities / Defend / Dice
	AbilityMenu,       // 어빌리티 목록
	SelectingTarget,   // 단일 타겟 선택
	SelectingTargetAll,// 전체 타겟 선택 (확인만 누르면 됨)
	DicePreview,       // 타겟 선택 후 주사위 굴리기 전 미리보기
	DiceManagement     // 현재 장착 주사위 확인/변경
};

/**
 * 키보드 네비게이션 배틀 메뉴.
 * NativeOnKeyDown에서 방향키/확인/취소 처리.
 * 실제 버튼 레이아웃과 Hover 설명 표시는 Blueprint에서 구현.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTTB_API UBattleMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ─── BattleManager 참조 (HUD에서 주입) ──────────────────────────────────
	UPROPERTY(BlueprintReadWrite, Category="Battle")
	TObjectPtr<ABattleManager> BattleManager;

	// ─── 현재 상태 ──────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadWrite, Category="Menu")
	EMenuState MenuState = EMenuState::Hidden;

	UPROPERTY(BlueprintReadWrite, Category="Menu")
	int32 SelectedIndex = 0;

	// ─── 메뉴 표시 제어 (C++ 호출 → Blueprint 구현) ──────────────────────────
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Menu")
	void ShowMainMenu(ABattleCombatant* ActiveCombatant);
	virtual void ShowMainMenu_Implementation(ABattleCombatant* ActiveCombatant);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Menu")
	void ShowAbilityMenu(const TArray<UTBGameplayAbility*>& Abilities);
	virtual void ShowAbilityMenu_Implementation(const TArray<UTBGameplayAbility*>& Abilities);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Menu")
	void ShowTargetSelection(const TArray<ABattleCombatant*>& Targets);
	virtual void ShowTargetSelection_Implementation(const TArray<ABattleCombatant*>& Targets);

	// 전체 타겟 선택 (AllEnemies, AllAllies) — 확인만 누르면 실행
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Menu")
	void ShowTargetSelectionAll(const TArray<ABattleCombatant*>& Targets);
	virtual void ShowTargetSelectionAll_Implementation(const TArray<ABattleCombatant*>& Targets);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Menu")
	void HideMenu();
	virtual void HideMenu_Implementation();

	// 주사위 미리보기 (타겟 선택 후, 굴리기 전)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Menu")
	void ShowDicePreview();
	virtual void ShowDicePreview_Implementation();

	// 주사위 관리 화면 (장착 주사위 확인/변경)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Menu")
	void ShowDiceManagement();
	virtual void ShowDiceManagement_Implementation();

	// ─── 입력 처리 ────────────────────────────────────────────────────────
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 현재 타겟 목록 (SelectingTarget 상태에서 사용)
	UPROPERTY(BlueprintReadOnly, Category="Menu")
	TArray<TObjectPtr<ABattleCombatant>> CurrentTargets;

	// DicePreview에서 사용할 선택된 타겟
	UPROPERTY(BlueprintReadOnly, Category="Menu")
	TObjectPtr<ABattleCombatant> PendingTarget;

	// 현재 턴 컴배턴트 (어빌리티 메뉴 진입 시 사용)
	UPROPERTY(BlueprintReadOnly, Category="Menu")
	TObjectPtr<ABattleCombatant> CurrentCombatant;

	// 어빌리티 목록 (AbilityMenu 상태에서 사용)
	UPROPERTY(BlueprintReadOnly, Category="Menu")
	TArray<TObjectPtr<UTBGameplayAbility>> CurrentAbilities;

	// 메인 메뉴 항목 수 (Attack=0, Abilities=1, Defend=2, Dice=3)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Menu")
	int32 MainMenuItemCount = 4;

	// DiceManagement용 주사위 목록 (GameInstance.OwnedDice 복사)
	UPROPERTY(BlueprintReadOnly, Category="Menu")
	TArray<FDiceData> CurrentDiceList;

protected:
	// 방향 입력 — C++에서 SelectedIndex 변경, Blueprint에서 시각적 처리 추가 가능
	UFUNCTION(BlueprintNativeEvent, Category="Menu") void NavigateUp();
	virtual void NavigateUp_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category="Menu") void NavigateDown();
	virtual void NavigateDown_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category="Menu") void NavigateLeft();
	virtual void NavigateLeft_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category="Menu") void NavigateRight();
	virtual void NavigateRight_Implementation();

	// 확인 / 취소 — C++에서 BattleManager 호출
	UFUNCTION(BlueprintNativeEvent, Category="Menu") void ConfirmSelection();
	virtual void ConfirmSelection_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category="Menu") void CancelSelection();
	virtual void CancelSelection_Implementation();

	// 현재 SelectedIndex에 맞춰 타겟 인디케이터 갱신
	void UpdateTargetIndicators();

	// 어빌리티 코스트를 지불할 수 있는지 체크
	UFUNCTION(BlueprintCallable, Category="Menu")
	bool CanAffordAbility(const UTBGameplayAbility* Ability) const;
};
