// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ArtifactDataTypes.h"
#include "PurchaseStepsWidget.generated.h"

class URichTextBlock;
class UTextBlock;
class UImage;
class USalesProductWidget;

UCLASS()
class PROJECTTB_API UPurchaseStepsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Shop")
	void InitializeDetailWidget(const FArtifactEntry& InArtifactEntry, USalesProductWidget* InOwningSalesProductWidget);
	
	UFUNCTION(BlueprintPure, Category="Shop")
	FArtifactEntry GetArtifactEntry() const { return ArtifactEntry; }

	UFUNCTION(BlueprintCallable, Category="Shop")
	bool TryPurchaseProduct();

	UFUNCTION(BlueprintCallable, Category="Shop")
	void CloseDetailWidget();

protected:
	//UFUNCTION(BlueprintImplementableEvent, Category="Shop")
	//void OnArtifactEntryAssigned(const FArtifactEntry& InArtifactEntry);

	UFUNCTION(BlueprintImplementableEvent, Category="Shop")
	void OnPurchaseSucceeded(const FArtifactEntry& PurchasedArtifact);

	UFUNCTION(BlueprintImplementableEvent, Category="Shop")
	void OnPurchaseFailed(const FArtifactEntry& FailedArtifact);
	

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> ArtifactIcon;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ArtifactNameText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> PriceText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<URichTextBlock> DescriptionText;
	
	
private:
	UPROPERTY(BlueprintReadOnly, Category="Shop", meta=(AllowPrivateAccess="true"))
	FArtifactEntry ArtifactEntry;

	UPROPERTY()
	TObjectPtr<USalesProductWidget> OwningSalesProductWidget;
};
