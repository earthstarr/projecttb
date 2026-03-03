#include "UI/BattleMenuWidget.h"
#include "Battle/BattleManager.h"
#include "Battle/BattleCombatant.h"
#include "Abilities/TBGameplayAbility.h"
#include "Framework/Application/SlateApplication.h"

void UBattleMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 키보드 포커스를 받을 수 있도록 설정
	SetIsFocusable(true);
}

FReply UBattleMenuWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 이 함수는 위젯 영역 내에서만 호출됨 — 뷰포트 클릭 시에는 호출 안됨
	UE_LOG(LogTemp, Warning, TEXT("BattleMenuWidget::NativeOnMouseButtonDown called"));
	if (APlayerController* PC = GetOwningPlayer())
	{
		SetUserFocus(PC);
	}
	return FReply::Handled();
}

FReply UBattleMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if      (Key == EKeys::Up    || Key == EKeys::W) { NavigateUp();        return FReply::Handled(); }
	else if (Key == EKeys::Down  || Key == EKeys::S) { NavigateDown();      return FReply::Handled(); }
	else if (Key == EKeys::Left  || Key == EKeys::A) { NavigateLeft();      return FReply::Handled(); }
	else if (Key == EKeys::Right || Key == EKeys::D) { NavigateRight();     return FReply::Handled(); }
	else if (Key == EKeys::Enter || Key == EKeys::SpaceBar)  { ConfirmSelection(); return FReply::Handled(); }
	else if (Key == EKeys::Escape|| Key == EKeys::BackSpace) { CancelSelection();  return FReply::Handled(); }

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// ─── 상태 관리 ────────────────────────────────────────────────────────────────

void UBattleMenuWidget::ShowMainMenu_Implementation(ABattleCombatant* ActiveCombatant)
{
	MenuState     = EMenuState::MainMenu;
	SelectedIndex = 0;
	CurrentCombatant = ActiveCombatant;
	CurrentTargets.Reset();
	CurrentAbilities.Reset();
}

void UBattleMenuWidget::ShowAbilityMenu_Implementation(const TArray<UTBGameplayAbility*>& Abilities)
{
	CurrentAbilities.Reset();
	for (UTBGameplayAbility* A : Abilities) CurrentAbilities.Add(A);
	MenuState     = EMenuState::AbilityMenu;
	SelectedIndex = 0;
}

void UBattleMenuWidget::ShowTargetSelection_Implementation(const TArray<ABattleCombatant*>& Targets)
{
	CurrentTargets.Reset();
	for (ABattleCombatant* T : Targets) CurrentTargets.Add(T);

	// SelectingTargetAll에서 호출된 경우 MenuState를 덮어쓰지 않음
	if (MenuState != EMenuState::SelectingTargetAll)
	{
		MenuState = EMenuState::SelectingTarget;
		SelectedIndex = 0;
	}

	UpdateTargetIndicators();
}

void UBattleMenuWidget::ShowTargetSelectionAll_Implementation(const TArray<ABattleCombatant*>& Targets)
{
	// 상태 먼저 설정 — ShowTargetSelection_Implementation에서 덮어쓰지 않도록
	MenuState = EMenuState::SelectingTargetAll;
	SelectedIndex = -1;  // Blueprint에서 전체 선택임을 인식 (< 0이면 모든 타겟 하이라이트)

	// Blueprint의 ShowTargetSelection 이벤트 호출 (UI 표시용)
	// ShowTargetSelection_Implementation은 MenuState가 SelectingTargetAll이면 덮어쓰지 않음
	ShowTargetSelection(Targets);
}

void UBattleMenuWidget::HideMenu_Implementation()
{
	for (ABattleCombatant* T : CurrentTargets)
		if (T) T->HideTargetIndicator();

	MenuState = EMenuState::Hidden;
	CurrentTargets.Reset();
}

// ─── 방향키 네비게이션 ────────────────────────────────────────────────────────

void UBattleMenuWidget::NavigateUp_Implementation()
{
	SelectedIndex = FMath::Max(0, SelectedIndex - 1);
}

void UBattleMenuWidget::NavigateDown_Implementation()
{
	int32 MaxIndex = MainMenuItemCount - 1;
	if      (MenuState == EMenuState::SelectingTarget) MaxIndex = CurrentTargets.Num() - 1;
	else if (MenuState == EMenuState::AbilityMenu)     MaxIndex = CurrentAbilities.Num() - 1;
	SelectedIndex = FMath::Min(FMath::Max(0, MaxIndex), SelectedIndex + 1);
}

void UBattleMenuWidget::NavigateLeft_Implementation()
{
	// 타겟 선택 시 왼쪽 = 이전 타겟
	if (MenuState == EMenuState::SelectingTarget)
	{
		SelectedIndex = FMath::Max(0, SelectedIndex - 1);
		UpdateTargetIndicators();
	}
}

void UBattleMenuWidget::NavigateRight_Implementation()
{
	// 타겟 선택 시 오른쪽 = 다음 타겟
	if (MenuState == EMenuState::SelectingTarget)
	{
		SelectedIndex = FMath::Min(CurrentTargets.Num() - 1, SelectedIndex + 1);
		UpdateTargetIndicators();
	}
}

// ─── 확인 / 취소 ─────────────────────────────────────────────────────────────

void UBattleMenuWidget::ConfirmSelection_Implementation()
{
	if (!BattleManager) return;

	switch (MenuState)
	{
	case EMenuState::MainMenu:
		if (SelectedIndex == 0)
		{
			BattleManager->PlayerSelectAttack();
		}
		else if (SelectedIndex == 1 && CurrentCombatant)
		{
			// bShowInAbilityMenu=true인 어빌리티만 표시 (기본 공격 제외)
			TArray<UTBGameplayAbility*> Skills = CurrentCombatant->GetGrantedAbilities().FilterByPredicate(
				[](const UTBGameplayAbility* A) { return A && A->bShowInAbilityMenu; });
			ShowAbilityMenu(Skills);
		}
		else if (SelectedIndex == 2)
		{
			BattleManager->PlayerSelectDefend();
		}
		break;

	case EMenuState::AbilityMenu:
		if (CurrentAbilities.IsValidIndex(SelectedIndex))
			BattleManager->PlayerSelectAbility(CurrentAbilities[SelectedIndex]->GetClass());
		break;

	case EMenuState::SelectingTarget:
		if (CurrentTargets.IsValidIndex(SelectedIndex))
			BattleManager->PlayerSelectTarget(CurrentTargets[SelectedIndex]);
		break;

	case EMenuState::SelectingTargetAll:
		// 전체 타겟 — 첫 번째 타겟을 넘기면 BattleManager에서 All 처리
		if (!CurrentTargets.IsEmpty())
			BattleManager->PlayerSelectTarget(CurrentTargets[0]);
		break;

	default:
		break;
	}
}

void UBattleMenuWidget::UpdateTargetIndicators()
{
	for (int32 i = 0; i < CurrentTargets.Num(); ++i)
	{
		if (!CurrentTargets[i]) continue;
		// SelectedIndex < 0 이면 AllTarget — 모두 표시
		if (SelectedIndex < 0 || i == SelectedIndex)
			CurrentTargets[i]->ShowTargetIndicator();
		else
			CurrentTargets[i]->HideTargetIndicator();
	}
}

void UBattleMenuWidget::CancelSelection_Implementation()
{
	if (!BattleManager) return;

	if (MenuState == EMenuState::AbilityMenu)
		ShowMainMenu(CurrentCombatant);  // 어빌리티 메뉴 → 메인 메뉴로 복귀
	else if (MenuState == EMenuState::SelectingTarget || MenuState == EMenuState::SelectingTargetAll)
		BattleManager->PlayerCancel();
	else
		BattleManager->PlayerCancel();
}
