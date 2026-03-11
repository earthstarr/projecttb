// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/World/PurchaseStepsWidget.h"

#include "TBGameInstance.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "UI/World/SalesProductWidget.h"

void UPurchaseStepsWidget::InitializeDetailWidget(const FArtifactEntry& InArtifactEntry, USalesProductWidget* InOwningSalesProductWidget)
{
	ArtifactEntry = InArtifactEntry;
	OwningSalesProductWidget = InOwningSalesProductWidget;

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
	
	//OnArtifactEntryAssigned(ArtifactEntry);
}

bool UPurchaseStepsWidget::TryPurchaseProduct()
{
	UTBGameInstance* TBGameInstance = Cast<UTBGameInstance>(GetGameInstance());
	if (TBGameInstance == nullptr)
	{
		OnPurchaseFailed(ArtifactEntry);
		return false;
	}

	if (TBGameInstance->HasArtifact(ArtifactEntry.ArtifactID))
	{
		OnPurchaseFailed(ArtifactEntry);
		return false;
	}

	if (!TBGameInstance->SpendGold(ArtifactEntry.ArtifactData.Price))
	{
		OnPurchaseFailed(ArtifactEntry);
		return false;
	}

	TBGameInstance->EquipArtifact(ArtifactEntry.ArtifactID);
	
	if (OwningSalesProductWidget)
	{
		OwningSalesProductWidget->HandlePurchaseSucceeded(ArtifactEntry.ArtifactID);
	}
	
	OnPurchaseSucceeded(ArtifactEntry);
	CloseDetailWidget();
	return true;
}

void UPurchaseStepsWidget::CloseDetailWidget()
{
	RemoveFromParent();
}
