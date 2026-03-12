// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/World/SpendMoneyWidget.h"

#include "TBGameInstance.h"

void USpendMoneyWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	TBInstance = Cast<UTBGameInstance>(GetGameInstance());
}

bool USpendMoneyWidget::TrySpendMoney(int32 Amount)
{
	if (TBInstance)
	{
		return TBInstance->SpendMoney(Amount);
	}
	return false;
}
