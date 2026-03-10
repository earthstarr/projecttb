// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/World/ShopWidget.h"

#include "TBGameInstance.h"
#include "Components/UniformGridPanel.h"
#include "UI/World/SalesProductWidget.h"

void UShopWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	TBGameInstance = Cast<UTBGameInstance>(GetGameInstance());
	
	MaxProductCnt = MaxRow * MaxCol;
	
	RefreshShopProducts();
}

void UShopWidget::RefreshShopProducts()
{
	if (TBGameInstance == nullptr || SalesProductGrid == nullptr || SalesProductWidgetClass == nullptr)
	{
		return;
	}

	SalesProductGrid->ClearChildren();

	const TArray<FArtifactEntry> UnownedArtifacts = TBGameInstance->GetUnownedArtifacts();

	int Col = 0,Row = 0;
	
	for (const FArtifactEntry& Entry : UnownedArtifacts)
	{
		
	}
	
	for (const FArtifactEntry& Entry : UnownedArtifacts)
	{
		USalesProductWidget* ProductWidget = CreateWidget<USalesProductWidget>(this, SalesProductWidgetClass);
		if (ProductWidget == nullptr)
		{
			continue;
		}
		ProductWidget->SetupProduct(Entry);
		SalesProductGrid->AddChildToUniformGrid(ProductWidget, Row, Col);
		Col++;
		
		// 제한된 수량만큼 상품 등록
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