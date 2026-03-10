// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ArtifactDataTypes.h"
#include "SalesProductWidget.generated.h"

class UTextBlock;
class UImage;
/**
 * 
 */
UCLASS()
class PROJECTTB_API USalesProductWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="Shop")
	void SetupProduct(const FArtifactEntry& InArtifactEntry);

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> ArtifactIcon;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ArtifactNameText;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> PriceText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

private:
	UPROPERTY(BlueprintReadOnly, Category="Shop", meta=(AllowPrivateAccess="true"))
	FArtifactEntry ArtifactEntry;
};
