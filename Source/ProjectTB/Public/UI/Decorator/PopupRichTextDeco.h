// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlockDecorator.h"
#include "PopupRichTextDeco.generated.h"

class FPopupRichTextDeco;

/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class PROJECTTB_API UPopupRichTextDeco : public URichTextBlockDecorator
{
	GENERATED_BODY()
	
public:
	// 에디터에서 선택할 '클릭 가능한 버튼 위젯' 클래스
	UPROPERTY(EditAnywhere, Category = "Appearance")
	TSubclassOf<UUserWidget> PopupWidgetClass;
	
	virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* InOwner) override;
};

class FPopupRichTextDeco : public URichTextBlockDecorator
{
public:
	FPopupRichTextDeco(URichTextBlock* InOwner, UPopupRichTextDeco* InDecorator)
						: FRichTextBlockDecorator(InOwner), Owner(InOwner), Decorator(InDecorator){}
};