#include "UI/LevelUpWidget.h"
#include "UI/TBBattleHUD.h"
#include "TimerManager.h"

void ULevelUpWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
}

void ULevelUpWidget::Initialize(const TArray<FLevelUpDisplayData>& InLevelUpData)
{
	LevelUpData   = InLevelUpData;
	bInputEnabled = false;

	// 1초 후 입력 활성화
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InputEnableTimerHandle,
			this,
			&ULevelUpWidget::EnableInput,
			1.0f,
			false
		);
	}
}

void ULevelUpWidget::EnableInput()
{
	bInputEnabled = true;

	// 포커스 설정
	if (APlayerController* PC = GetOwningPlayer())
	{
		SetUserFocus(PC);
	}
}

FReply ULevelUpWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!bInputEnabled)
		return FReply::Handled();

	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
	{
		OnConfirm();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void ULevelUpWidget::OnConfirm_Implementation()
{
	if (!OwningHUD) return;

	// 월드 복귀
	OwningHUD->ReturnToWorld();

	// 이 위젯 숨기기
	RemoveFromParent();
}
