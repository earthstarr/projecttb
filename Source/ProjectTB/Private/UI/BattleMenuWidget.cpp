#include "UI/BattleMenuWidget.h"
#include "Battle/BattleManager.h"
#include "Battle/BattleCombatant.h"
#include "Battle/BattlePlayerCharacter.h"
#include "Abilities/TBGameplayAbility.h"
#include "TBGameInstance.h"
#include "Framework/Application/SlateApplication.h"

void UBattleMenuWidget::NativeConstruct()
{
	UE_LOG(LogTemp, Log, TEXT("UBattleMenuWidget::NativeConstruct Enter"));

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
	UE_LOG(LogTemp, Log, TEXT("UBattleMenuWidget::NativeOnKeyDown Enter, MenuState=%d"), (int32)MenuState);

	const FKey Key = InKeyEvent.GetKey();

	// 패링 (F키) — 메뉴 상태와 관계없이 항상 처리
	if (Key == EKeys::F)
	{
		if (BattleManager)
			BattleManager->TryParry();
		return FReply::Handled();
	}

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
	UE_LOG(LogTemp, Log, TEXT("UBattleMenuWidget::ShowMainMenu_Implementation Enter"));

	// 타겟 선택 중 메인 메뉴로 복귀 시 인디케이터 정리
	for (ABattleCombatant* T : CurrentTargets)
		if (T) T->HideTargetIndicator();

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

void UBattleMenuWidget::ShowDicePreview_Implementation()
{
	MenuState     = EMenuState::DicePreview;
	SelectedIndex = 0;
}

void UBattleMenuWidget::ShowDiceManagement_Implementation()
{
	// GameInstance에서 OwnedDice 복사
	CurrentDiceList.Reset();
	if (UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance()))
	{
		CurrentDiceList = GI->OwnedDice;
	}

	// 현재 캐릭터의 장착 주사위 인덱스로 초기 선택
	if (ABattlePlayerCharacter* PC = Cast<ABattlePlayerCharacter>(CurrentCombatant))
	{
		SelectedIndex = PC->GetEquippedDiceIndex();
	}
	else
	{
		SelectedIndex = 0;
	}

	MenuState = EMenuState::DiceManagement;
}

// ─── 방향키 네비게이션 ────────────────────────────────────────────────────────

void UBattleMenuWidget::NavigateUp_Implementation()
{
	SelectedIndex = FMath::Max(0, SelectedIndex - 1);
}

void UBattleMenuWidget::NavigateDown_Implementation()
{
	int32 MaxIndex = MainMenuItemCount - 1;
	if      (MenuState == EMenuState::SelectingTarget)  MaxIndex = CurrentTargets.Num() - 1;
	else if (MenuState == EMenuState::AbilityMenu)      MaxIndex = CurrentAbilities.Num() - 1;
	else if (MenuState == EMenuState::DiceManagement)   MaxIndex = CurrentDiceList.Num() - 1;
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
			// bShowInAbilityMenu=true이고 레벨 조건을 만족하는 어빌리티만 표시
			const int32 CharLevel = Cast<ABattlePlayerCharacter>(CurrentCombatant)
				? Cast<ABattlePlayerCharacter>(CurrentCombatant)->CharacterLevel : 1;
			TArray<UTBGameplayAbility*> Skills = CurrentCombatant->GetGrantedAbilities().FilterByPredicate(
				[CharLevel](const UTBGameplayAbility* A)
				{
					return A && A->bShowInAbilityMenu && A->RequiredLevel <= CharLevel;
				});
			ShowAbilityMenu(Skills);
		}
		else if (SelectedIndex == 2)
		{
			BattleManager->PlayerSelectDefend();
		}
		else if (SelectedIndex == 3)
		{
			ShowDiceManagement();
		}
		break;

	case EMenuState::AbilityMenu:
		if (CurrentAbilities.IsValidIndex(SelectedIndex) && CurrentCombatant)
		{
			UTBGameplayAbility* SelectedAbility = CurrentAbilities[SelectedIndex];
			if (SelectedAbility && CanAffordAbility(SelectedAbility))
			{
				BattleManager->PlayerSelectAbility(SelectedAbility->GetClass());
			}
			// 코스트 부족 시 무시 (아무것도 안 함)
		}
		break;

	case EMenuState::SelectingTarget:
		if (CurrentTargets.IsValidIndex(SelectedIndex))
		{
			// 타겟 저장 후 DicePreview로 전환
			PendingTarget = CurrentTargets[SelectedIndex];
			ShowDicePreview();
		}
		break;

	case EMenuState::SelectingTargetAll:
		// 전체 타겟 — 첫 번째 타겟 저장 후 DicePreview로 전환
		if (!CurrentTargets.IsEmpty())
		{
			PendingTarget = CurrentTargets[0];
			ShowDicePreview();
		}
		break;

	case EMenuState::DicePreview:
		// "굴리기" 확인 → 저장된 타겟으로 액션 실행
		if (PendingTarget)
			BattleManager->PlayerSelectTarget(PendingTarget);
		break;

	case EMenuState::DiceManagement:
		// 선택한 주사위 장착
		if (CurrentDiceList.IsValidIndex(SelectedIndex))
		{
			if (ABattlePlayerCharacter* PC = Cast<ABattlePlayerCharacter>(CurrentCombatant))
			{
				PC->EquipDice(SelectedIndex);
			}
			// 메인 메뉴로 복귀
			ShowMainMenu(CurrentCombatant);
		}
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

	if (MenuState == EMenuState::AbilityMenu || MenuState == EMenuState::DiceManagement)
		ShowMainMenu(CurrentCombatant);  // 서브 메뉴 → 메인 메뉴로 복귀
	else if (MenuState == EMenuState::DicePreview)
	{
		// DicePreview → 타겟 선택으로 복귀
		PendingTarget = nullptr;
		ShowTargetSelection(TArray<ABattleCombatant*>(CurrentTargets));
	}
	else if (MenuState == EMenuState::SelectingTarget || MenuState == EMenuState::SelectingTargetAll)
		BattleManager->PlayerCancel();
	else
		BattleManager->PlayerCancel();
}

bool UBattleMenuWidget::CanAffordAbility(const UTBGameplayAbility* Ability) const
{
	if (!Ability || !CurrentCombatant) return false;

	const float CostAmount = Ability->CostAmount;
	if (CostAmount <= 0.f) return true;  // 코스트 없음

	switch (Ability->CostType)
	{
	case EAbilityCostType::Stamina:
		return CurrentCombatant->GetStamina() >= CostAmount;
	case EAbilityCostType::MP:
		return CurrentCombatant->GetMP() >= CostAmount;
	case EAbilityCostType::HP:
		return CurrentCombatant->GetHP() > CostAmount;  // HP는 코스트보다 커야 함 (0 방지)
	case EAbilityCostType::None:
	default:
		return true;
	}
}
