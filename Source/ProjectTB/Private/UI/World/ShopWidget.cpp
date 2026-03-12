// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/World/ShopWidget.h"

#include "TBGameInstance.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "UI/Artifact/OwnedArtifactListWidget.h"
#include "UI/World/SalesProductWidget.h"
#include "World/LevelActor/Shop/ShopkeeperBasePawn.h"

void UShopWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	ensure(SalesProductWidgetClass);
}

void UShopWidget::RefreshShopProducts()
{
	if (OwningShopkeeper == nullptr || SalesProductGrid == nullptr || SalesProductWidgetClass == nullptr)
	{
		return;
	}
	
	if (Money != nullptr && TBGameInstance != nullptr)
	{
		Money->SetText(FText::AsNumber(TBGameInstance->GetCurrentMoney()));
	}
	
	
	SalesProductGrid->ClearChildren();

	const TArray<FArtifactEntry>& Entries = OwningShopkeeper->GetShopProductEntries();

	int Col = 0,Row = 0;
	
	for (const FArtifactEntry& Entry : Entries)
	{
		USalesProductWidget* ProductWidget = CreateWidget<USalesProductWidget>(this, SalesProductWidgetClass);
		if (ProductWidget == nullptr)
		{
			continue;
		}

		const bool bSoldOut = OwningShopkeeper->IsSoldOut(Entry.ArtifactID);
		ProductWidget->SetupProduct(Entry, this);
		ProductWidget->SetSoldOut(bSoldOut);

		SalesProductGrid->AddChildToUniformGrid(ProductWidget, Row, Col);

		// 제한된 수량만큼 상품 등록
		Col++;
		
		if (Col >= MaxCol)
		{
			Row++;
			Col = 0;
			if (Row >= MaxRow)
			{
				break;
			}
		}
	}
}

void UShopWidget::InitializeShop(AShopkeeperBasePawn* InShopkeeper)
{
	TBGameInstance = Cast<UTBGameInstance>(GetGameInstance());
	if (TBGameInstance == nullptr)
	{
		UE_LOG(LogTemp,Warning,TEXT("UShopWidget::InitializeShop 의 TBGameInstance 가 nullptr 입니다."));
	}
	
	
	if (InShopkeeper == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UShopWidget의 InitializeShop함수 InShopkeeper 이 없습니다."));
		return;
	}
	OwningShopkeeper = InShopkeeper;
	OwningShopkeeper->InitializeShopInventory();
	RefreshShopProducts();
}

void UShopWidget::HandleProductPurchased(FName PurchasedArtifactID)
{
	if (OwningShopkeeper == nullptr)
	{
		return;
	}

	OwningShopkeeper->MarkProductSoldOut(PurchasedArtifactID);
	
	RefreshShopProducts();
}

void UShopWidget::ShowShopWidget()
{
	SetVisibility(ESlateVisibility::Visible);
	
	if (EquippedArtifactList)
	{
		EquippedArtifactList->RefreshOwnedArtifacts();
	}

	RefreshShopProducts();
}

void UShopWidget::HideShopWidget()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
