// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PopupWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTTB_API UPopupWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="DataTable")
	FDataTableRowHandle TextRowHandle;
	
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	//포커스 잃으면 닫히기
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;
	
	void SetPopupText(FString Key);
	
};
