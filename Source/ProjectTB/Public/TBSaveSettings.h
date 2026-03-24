#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "TBSaveSettings.generated.h"

UCLASS()
class PROJECTTB_API UTBSaveSettings : public USaveGame
{
	GENERATED_BODY()

public:
	// Audio
	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float MasterVolume = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float MusicVolume = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float SFXVolume = 1.0f;

	// Brightness (UGameUserSettings에 없어서 별도 저장)
	UPROPERTY(BlueprintReadWrite, Category = "Video")
	float Brightness = 50.0f;
};
