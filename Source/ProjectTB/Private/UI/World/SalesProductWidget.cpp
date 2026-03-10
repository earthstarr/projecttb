// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/World/SalesProductWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void USalesProductWidget::SetupProduct(const FArtifactEntry& InArtifactEntry)
{
	ArtifactEntry = InArtifactEntry;

	if (ArtifactNameText)
	{
		ArtifactNameText->SetText(ArtifactEntry.ArtifactData.DisplayName);
	}

	if (PriceText)
	{
		PriceText->SetText(FText::AsNumber(ArtifactEntry.ArtifactData.Price));
	}

	if (DescriptionText)
	{
		DescriptionText->SetText(ArtifactEntry.ArtifactData.Description);
	}

	if (ArtifactIcon)
	{
		if (UTexture2D* IconTexture = ArtifactEntry.ArtifactData.Icon.LoadSynchronous())
		{
			ArtifactIcon->SetBrushFromTexture(IconTexture);
		}
	}
}