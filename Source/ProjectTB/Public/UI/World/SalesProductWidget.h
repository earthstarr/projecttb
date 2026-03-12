// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ArtifactDataTypes.h"
#include "SalesProductWidget.generated.h"

class UBorder;
class UTextBlock;
class UImage;
class UShopWidget;
class UPurchaseStepsWidget;
/**
 * 
 */
UCLASS()
class PROJECTTB_API USalesProductWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="Shop")
	void SetupProduct(const FArtifactEntry& InArtifactEntry, UShopWidget* InOwningShopWidget);

	UFUNCTION(BlueprintCallable, Category="SalesProduct")
	UPurchaseStepsWidget* OpenPurchaseStepsWidget();

	UFUNCTION(BlueprintCallable, Category="Shop")
	void SetSoldOut(bool bInSoldOut);

	UFUNCTION(BlueprintCallable, Category="Shop")
	void HandlePurchaseSucceeded(FName PurchasedArtifactID);
	
protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> ArtifactIcon;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ArtifactNameText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> PriceText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UBorder> SoldOutBorder;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop")
	TSubclassOf<UPurchaseStepsWidget> ProductDetailWidgetClass;

private:
	UPROPERTY(BlueprintReadOnly, Category="Shop", meta=(AllowPrivateAccess="true"))
	FArtifactEntry ArtifactEntry;

	UPROPERTY()
	TObjectPtr<UShopWidget> OwningShopWidget;

	UPROPERTY(Transient)
	TObjectPtr<UPurchaseStepsWidget> ActiveDetailWidget;
	
	bool bSoldOut = false;
};
