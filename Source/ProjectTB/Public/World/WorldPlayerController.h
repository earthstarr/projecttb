// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WorldPlayerController.generated.h"

class ULevelUpWidget;
/**
 * 
 */
class UOwnedArtifactListWidget;

UCLASS()
class PROJECTTB_API AWorldPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	void ShowWidget(UUserWidget* Widget, bool bIgnoreMoveInput);
	void CloseWidget(UUserWidget* Widget, bool bActiveMoveInput);
	void SetInputModeGameOnly();
	void SetInputModeUIOnly();
	void SetInputModeBattle();
	void SetInputModeWorld();
	void ToggleWorldUIMode();
	void TogglePartyStatus();
	
	// 층 수 조절하는 콘솔 명령어
public:
	UFUNCTION(Exec)
	void PortalSetCount(int32 NewCount);

	UFUNCTION(Exec)
	void PortalAddCount(int32 Delta);

	UFUNCTION(Exec)
	void PortalPrintCount() const;
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI", Meta = (AllowPrivateAccess = true))
	TSubclassOf<UOwnedArtifactListWidget> OwnedArtifactListWidgetClass;

	UPROPERTY(Meta = (AllowPrivateAccess = true))
	TObjectPtr<UOwnedArtifactListWidget> OwnedArtifactListWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI", Meta=(AllowPrivateAccess=true))
	TSubclassOf<ULevelUpWidget> PartyStatusWidgetClass;

	UPROPERTY(Meta=(AllowPrivateAccess=true))
	TObjectPtr<ULevelUpWidget> PartyStatusWidget;
	
	UPROPERTY(Meta = (AllowPrivateAccess = true))
	TObjectPtr<UUserWidget> CurrentWidget;
	
	UPROPERTY(Meta = (AllowPrivateAccess = true))
	bool bWorldUIMode = false;
	
};
