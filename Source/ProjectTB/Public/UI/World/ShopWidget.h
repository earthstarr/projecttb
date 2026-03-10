// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShopWidget.generated.h"

class UTBGameInstance;
class USalesProductWidget;
class UUniformGridPanel;
/**
 * 
 */
UCLASS()
class PROJECTTB_API UShopWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	
	UFUNCTION(BlueprintCallable, Category="Shop")
	void RefreshShopProducts();
	
protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UUniformGridPanel> SalesProductGrid;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop")
	TSubclassOf<USalesProductWidget> SalesProductWidgetClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	int32 MaxCol = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	int32 MaxRow = 1;
	
private:
	UPROPERTY()
	TObjectPtr<UTBGameInstance> TBGameInstance;
};
