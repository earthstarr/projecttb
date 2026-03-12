// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/World/SalesProductWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "UI/World/PurchaseStepsWidget.h"
#include "UI/World/ShopWidget.h"

void USalesProductWidget::SetupProduct(const FArtifactEntry& InArtifactEntry, UShopWidget* InOwningShopWidget)
{
	ArtifactEntry = InArtifactEntry;
	OwningShopWidget = InOwningShopWidget;


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

UPurchaseStepsWidget* USalesProductWidget::OpenPurchaseStepsWidget()
{
	if (ProductDetailWidgetClass == nullptr)
	{
		return nullptr;
	}

	if (ActiveDetailWidget)
	{
		ActiveDetailWidget->RemoveFromParent();
		ActiveDetailWidget = nullptr;
	}

	if (bSoldOut == true)
	{
		UE_LOG(LogTemp,Warning,TEXT("USalesProductWidget::OpenPurchaseStepsWidget bSoldOut이 true 인 상황에서 상세 설명창을 열려고 했습니다. SoldOutBorder 가 나오지 않았는지 확인해야합니다."))
		return nullptr;
	}
	
	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController == nullptr)
	{
		PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	}

	if (PlayerController == nullptr)
	{
		return nullptr;
	}

	ActiveDetailWidget = CreateWidget<UPurchaseStepsWidget>(PlayerController, ProductDetailWidgetClass);
	if (ActiveDetailWidget == nullptr)
	{
		return nullptr;
	}

	ActiveDetailWidget->InitializeDetailWidget(ArtifactEntry, this);
	ActiveDetailWidget->AddToViewport(100);

	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	//ActiveDetailWidget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	//ActiveDetailWidget->SetPositionInViewport(ViewportSize * 0.5f, false);

	return ActiveDetailWidget;
}

void USalesProductWidget::SetSoldOut(bool bInSoldOut)
{
	bSoldOut = bInSoldOut;

	if (SoldOutBorder)
	{
		SoldOutBorder->SetVisibility(bSoldOut ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void USalesProductWidget::HandlePurchaseSucceeded(FName PurchasedArtifactID)
{
	if (ArtifactEntry.ArtifactID != PurchasedArtifactID)
	{
		return;
	}

	if (OwningShopWidget)
	{
		OwningShopWidget->HandleProductPurchased(PurchasedArtifactID);
	}
}
