#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WorldGameMode.generated.h"

/**
 * 월드(탐험) 씬 전용 게임모드.
 * Default Pawn = BP_WorldCharacter (에디터에서 설정).
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTTB_API AWorldGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWorldGameMode();
};
