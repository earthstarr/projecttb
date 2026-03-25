#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PortalFloorWidget.generated.h"

class UTextBlock;

UCLASS()
class PROJECTTB_API UPortalFloorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UPROPERTY(BlueprintReadOnly, Category="Portal")
	int32 CurrentPortalMoveCount = 0;

	UFUNCTION(BlueprintImplementableEvent, Category="Portal")
	void ReceivePortalMoveCountChanged(int32 NewPortalMoveCount);

private:
	UFUNCTION()
	void HandlePortalMoveCountChanged(int32 NewPortalMoveCount);

	void RefreshFloorText(int32 PortalMoveCount);
};
