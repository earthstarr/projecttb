// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ArtifactDataTypes.h"
#include "ArtifactDescriptionPopupWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class URichTextBlock;
class UTextBlock;

UCLASS()
class PROJECTTB_API UArtifactDescriptionPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	void InitializePopup(const FArtifactEntry& InArtifactEntry, const FVector2D& ClickPosition);

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> BackgroundCloseButton;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UBorder> PopupBody;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ArtifactNameText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<URichTextBlock> DescriptionText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Artifact|Layout", meta=(ClampMin="0.0"))
	float DescriptionWrapTextAt = 0.0f;

private:
	UFUNCTION()
	void HandleBackgroundCloseClicked();

	void ApplyDescriptionTextLayout();
	void PositionPopup(const FVector2D& ClickPosition);

	UPROPERTY()
	FArtifactEntry ArtifactEntry;
};
