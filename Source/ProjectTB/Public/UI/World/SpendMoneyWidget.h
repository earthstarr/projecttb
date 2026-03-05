// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SpendMoneyWidget.generated.h"

class UTBGameInstance;
/**
 * 
 */
UCLASS()
class PROJECTTB_API USpendMoneyWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	
	
	UFUNCTION(BlueprintCallable)
	bool TrySpendMoney(int32 Amount);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Cost")
	int32 Cost;
	
private:
	UTBGameInstance* TBInstance;
};
