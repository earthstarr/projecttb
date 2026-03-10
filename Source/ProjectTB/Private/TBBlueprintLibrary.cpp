#include "TBBlueprintLibrary.h"

FString UTBBlueprintLibrary::GetDiceFacesAsString(const FDiceData& DiceData)
{
	return DiceData.GetFacesAsString();
}
