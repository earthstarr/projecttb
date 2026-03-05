// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlockDecorator.h"
#include "PopupRichTextDeco.generated.h"

class UPopupWidget;
class FPopupRichTextDeco;

/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class PROJECTTB_API UPopupRichTextDeco : public URichTextBlockDecorator
{
	GENERATED_BODY()
	
public:
	// 에디터에서 선택할 클릭 했을 때 나오는 위젯. PopupWidgetClass를 상속받을 것.
	UPROPERTY(EditAnywhere, Category = "Appearance")
	TSubclassOf<UPopupWidget> PopupWidgetClass;
	
	UFUNCTION(BlueprintImplementableEvent)
	void SetPopupID(const FString& PopupID);
	
	// FPopupRichTextDeco 생성
	virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* InOwner) override;
	
	// Popup 위젯 생성 or 교체
	void OpenPopup(URichTextBlock* OwnerRichText, const FString& PopupID);
	
private:
	//한번에 하나의 팝업만 나오도록 가장 최신 팝업을 기억
	TWeakObjectPtr<UPopupWidget> CurrentPopupWidget;
};

class FPopupRichTextDeco : public FRichTextDecorator
{
public:
	FPopupRichTextDeco(URichTextBlock* InOwner, UPopupRichTextDeco* InDecorator)
						: FRichTextDecorator(InOwner), Owner(InOwner), Decorator(InDecorator){};
	
	//파싱된 문자열이 찾고 있는 태그인지 확인
	virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override;
	
	//선택 가능한 위젯으로 변경
	virtual TSharedPtr<SWidget> CreateDecoratorWidget(const FTextRunInfo& RunInfo, const FTextBlockStyle& DefaultTextStyle) const override;
	
	//클릭시 떠오를 팝업창
	FReply OnPopupClicked(FString PopupID);
	
private:
	URichTextBlock* Owner;
	UPopupRichTextDeco* Decorator;
};