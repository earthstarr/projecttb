#include "UI/VictoryWidget.h"
#include "UI/TBBattleHUD.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

void UVictoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
}

void UVictoryWidget::Initialize(const TArray<FCharacterExpData>& InBeforeExpData,
                                 const TArray<FCharacterExpData>& InAfterExpData,
                                 const TArray<FLevelUpDisplayData>& InLevelUpData,
                                 int32 InRewardMoney)
{
	BeforeExpData  = InBeforeExpData;
	AfterExpData   = InAfterExpData;
	LevelUpData    = InLevelUpData;
	RewardMoney    = InRewardMoney;
	bHasLevelUp    = LevelUpData.Num() > 0;
	bShowingAfter  = false;
	bInputEnabled  = false;

	// 초기 표시: 획득 전 상태 + GainedExp만 AfterExpData에서 가져오기
	CurrentExpData = BeforeExpData;
	for (int32 i = 0; i < CurrentExpData.Num() && i < AfterExpData.Num(); ++i)
	{
		CurrentExpData[i].GainedExp = AfterExpData[i].GainedExp;
	}

	// 초기 UI 갱신
	RefreshUI();

	// 즉시 입력 활성화
	EnableInput();
}

void UVictoryWidget::EnableInput()
{
	bInputEnabled = true;

	// 포커스 설정
	if (APlayerController* PC = GetOwningPlayer())
	{
		SetUserFocus(PC);
	}
}

FReply UVictoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
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

void UVictoryWidget::OnConfirm_Implementation()
{
	if (!bShowingAfter)
	{
		// 첫 번째 엔터: 획득 후 상태로 전환
		bShowingAfter = true;
		CurrentExpData = AfterExpData;
		RefreshUI();
		return;
	}

	// 두 번째 엔터: 다음 단계로
	if (!OwningHUD) return;

	// 일시정지 해제 및 시간 정상화
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGamePaused(World, false);
		UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);
	}
	
	OwningHUD->ReturnToWorld();

	/*if (bHasLevelUp)
	{
		// 레벨업 위젯 표시
		OwningHUD->ShowLevelUpWidget(LevelUpData);
	}
	else
	{
		// 월드 복귀
		OwningHUD->ReturnToWorld();
	}*/

	// 이 위젯 숨기기
	RemoveFromParent();
}
