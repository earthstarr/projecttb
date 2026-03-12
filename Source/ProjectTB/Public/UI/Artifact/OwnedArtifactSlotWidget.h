// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ArtifactDataTypes.h"
#include "OwnedArtifactSlotWidget.generated.h"

class UButton;
class UImage;
class UOwnedArtifactListWidget;

UCLASS()
class PROJECTTB_API UOwnedArtifactSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void InitializeArtifactSlot(const FArtifactEntry& InArtifactEntry, UOwnedArtifactListWidget* InOwnerList);

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ArtifactButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> ArtifactIcon;

private:
	UFUNCTION()
	void HandleArtifactButtonClicked();

	UPROPERTY()
	TObjectPtr<UOwnedArtifactListWidget> OwnerListWidget;

	UPROPERTY()
	FArtifactEntry ArtifactEntry;
};
