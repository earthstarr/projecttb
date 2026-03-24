// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Artifact/ArtifactDescriptionPopupWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"

void UArtifactDescriptionPopupWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	ApplyDescriptionTextLayout();
}

void UArtifactDescriptionPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ApplyDescriptionTextLayout();

	if (BackgroundCloseButton)
	{
		BackgroundCloseButton->OnClicked.AddDynamic(this, &UArtifactDescriptionPopupWidget::HandleBackgroundCloseClicked);
	}
}

void UArtifactDescriptionPopupWidget::InitializePopup(const FArtifactEntry& InArtifactEntry, const FVector2D& ClickPosition)
{
	ArtifactEntry = InArtifactEntry;

	if (ArtifactNameText)
	{
		ArtifactNameText->SetText(ArtifactEntry.ArtifactData.DisplayName);
	}

	if (DescriptionText)
	{
		ApplyDescriptionTextLayout();
		DescriptionText->SetText(ArtifactEntry.ArtifactData.Description);
	}

	PositionPopup(ClickPosition);
}

void UArtifactDescriptionPopupWidget::HandleBackgroundCloseClicked()
{
	RemoveFromParent();
}

void UArtifactDescriptionPopupWidget::ApplyDescriptionTextLayout()
{
	if (DescriptionText == nullptr)
	{
		return;
	}

	DescriptionText->SetAutoWrapText(true);
	DescriptionText->SetWrapTextAt(DescriptionWrapTextAt);
}

void UArtifactDescriptionPopupWidget::PositionPopup(const FVector2D& ClickPosition)
{
	UE_LOG(LogTemp, Warning, TEXT("[ArtifactPopupDebug] PositionPopup skipped. PopupBody position is controlled by widget anchor. click=(%.2f, %.2f)"),
		ClickPosition.X, ClickPosition.Y);
}
