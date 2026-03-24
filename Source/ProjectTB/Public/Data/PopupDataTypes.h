#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PopupDataTypes.generated.h"

USTRUCT(BlueprintType)
struct FPopupDescriptionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Popup")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Popup")
	FText Description;
};
