#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Battle/TBDiceData.h"
#include "TBBlueprintLibrary.generated.h"

/**
 * ProjectTB용 Blueprint 유틸 함수 모음.
 */
UCLASS()
class PROJECTTB_API UTBBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// FDiceData의 BaseFaces를 문자열로 반환 (예: "[-3, -2, -1, 0, +1, +2, +3]")
	UFUNCTION(BlueprintPure, Category="Dice", meta=(DisplayName="Get Dice Faces As String"))
	static FString GetDiceFacesAsString(const FDiceData& DiceData);
};
