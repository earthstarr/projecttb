// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShopWidget.generated.h"

class UOwnedArtifactListWidget;
class AShopkeeperBasePawn;
struct FArtifactEntry;
class UTextBlock;
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

	UFUNCTION(BlueprintCallable, Category="Shop")
	void InitializeShop(AShopkeeperBasePawn* InShopkeeper);

	UFUNCTION(BlueprintCallable, Category="Shop")
	void HandleProductPurchased(FName PurchasedArtifactID);

	UFUNCTION(BlueprintCallable, Category="Shop")
	void ShowShopWidget();

	UFUNCTION(BlueprintCallable, Category="Shop")
	void HideShopWidget();

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UUniformGridPanel> SalesProductGrid;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop")
	TSubclassOf<USalesProductWidget> SalesProductWidgetClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	int32 MaxCol = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	int32 MaxRow = 1;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> Money;
	
	UPROPERTY(BlueprintReadOnly, Category="Shop")
	TArray<FArtifactEntry> ShopProductEntries;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Shop")
	TObjectPtr<UOwnedArtifactListWidget> EquippedArtifactList;
	
private:
	UPROPERTY()
	TObjectPtr<UTBGameInstance> TBGameInstance;
	
	UPROPERTY()
	TObjectPtr<AShopkeeperBasePawn> OwningShopkeeper;
};
