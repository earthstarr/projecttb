// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WorldPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTTB_API AWorldPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void ShowWidget(UUserWidget* Widget, bool bIgnoreMoveInput);
	void CloseWidget(UUserWidget* Widget, bool bActiveMoveInput);
	void SetInputModeGameOnly();
	void SetInputModeUIOnly();
	void SetInputModeBattle();
	void SetInputModeWorld();
	
private:
	UPROPERTY(Meta = (AllowPrivateAccess = true))
	UUserWidget* CurrentWidget;
};
