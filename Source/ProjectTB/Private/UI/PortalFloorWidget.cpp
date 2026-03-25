#include "UI/PortalFloorWidget.h"

#include "Components/TextBlock.h"
#include "TBGameInstance.h"

void UPortalFloorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance()))
	{
		GI->OnPortalMoveCountChanged.AddDynamic(this, &UPortalFloorWidget::HandlePortalMoveCountChanged);
		RefreshFloorText(GI->GetPortalMoveCount());
	}
}

void UPortalFloorWidget::NativeDestruct()
{
	if (UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance()))
	{
		GI->OnPortalMoveCountChanged.RemoveDynamic(this, &UPortalFloorWidget::HandlePortalMoveCountChanged);
	}

	Super::NativeDestruct();
}

void UPortalFloorWidget::HandlePortalMoveCountChanged(int32 NewPortalMoveCount)
{
	RefreshFloorText(NewPortalMoveCount);
}

void UPortalFloorWidget::RefreshFloorText(int32 PortalMoveCount)
{
	CurrentPortalMoveCount = PortalMoveCount;
	
	ReceivePortalMoveCountChanged(CurrentPortalMoveCount);
}
