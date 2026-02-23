#include "UI/BattleMenuWidget.h"
#include "Battle/BattleManager.h"
#include "Battle/BattleCombatant.h"
#include "Abilities/TBGameplayAbility.h"
#include "Framework/Application/SlateApplication.h"

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
	MenuState    = EMenuState::SelectingTarget;
	SelectedIndex = 0;
}

void UBattleMenuWidget::HideMenu_Implementation()
{
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
		SelectedIndex = FMath::Max(0, SelectedIndex - 1);
}

void UBattleMenuWidget::NavigateRight_Implementation()
{
	// 타겟 선택 시 오른쪽 = 다음 타겟
	if (MenuState == EMenuState::SelectingTarget)
		SelectedIndex = FMath::Min(CurrentTargets.Num() - 1, SelectedIndex + 1);
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
		break;

	case EMenuState::AbilityMenu:
		if (CurrentAbilities.IsValidIndex(SelectedIndex))
			BattleManager->PlayerSelectAbility(CurrentAbilities[SelectedIndex]->GetClass());
		break;

	case EMenuState::SelectingTarget:
		if (CurrentTargets.IsValidIndex(SelectedIndex))
			BattleManager->PlayerSelectTarget(CurrentTargets[SelectedIndex]);
		break;

	default:
		break;
	}
}

void UBattleMenuWidget::CancelSelection_Implementation()
{
	if (!BattleManager) return;

	if (MenuState == EMenuState::AbilityMenu)
		ShowMainMenu(CurrentCombatant);  // 어빌리티 메뉴 → 메인 메뉴로 복귀
	else
		BattleManager->PlayerCancel();
}
