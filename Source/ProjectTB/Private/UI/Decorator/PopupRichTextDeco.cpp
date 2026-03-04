// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Decorator/PopupRichTextDeco.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Blueprint/UserWidget.h"
#include "Components/RichTextBlock.h"
#include "UI/PopupWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"

TSharedPtr<ITextDecorator> UPopupRichTextDeco::CreateDecorator(URichTextBlock* InOwner)
{
	return MakeShareable(new FPopupRichTextDeco(InOwner, this));
}

void UPopupRichTextDeco::OpenPopup(URichTextBlock* OwnerRichText, const FString& PopupID)
{
	if (!OwnerRichText || !PopupWidgetClass)
	{
		return;
	}
	
	UWorld* World = OwnerRichText->GetWorld();
	
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPopupRichTextDeco 클래스 OpenPopup함수 UWorld is nullptr"));
		return;
	}
	
	// 이미 열려 있으면 기존 팝업 제거
	if (CurrentPopupWidget.IsValid())
	{
		CurrentPopupWidget->RemoveFromParent();
		CurrentPopupWidget = nullptr;
	}
	
	UPopupWidget* PopupWidget = CreateWidget<UPopupWidget>(World, PopupWidgetClass);
	
	if (PopupWidget)
	{
		PopupWidget->AddToViewport();
		//텍스트 내용 결정
		PopupWidget->SetPopupText(PopupID);
		
		//마우스 위치에 팝업창 생성
		UWidgetLayoutLibrary::GetMousePositionOnViewport(World);
		FVector2D MousePosition;
		MousePosition += FVector2D(20.f, 20.f);
		
		//화면 밖으로 나가지 않도록 적용
		FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(World);
		
		//위젯 크기 즉시 계산 요청
		PopupWidget->ForceLayoutPrepass();
		
		//위젯 크기 가져오기 추가해야함. todo
		FVector2D WidgetSize = PopupWidget->GetDesiredSize();
		
		MousePosition.X = FMath::Clamp(MousePosition.X, 0.f, ViewportSize.X - WidgetSize.X);
		MousePosition.Y = FMath::Clamp(MousePosition.Y, 0.f, ViewportSize.Y - WidgetSize.Y);

		PopupWidget->SetPositionInViewport(MousePosition);
		
		//현재 팝업 위젯 최신화
		CurrentPopupWidget = PopupWidget;
	}
}

bool FPopupRichTextDeco::Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const
{
	return RunParseResult.Name == TEXT("popup");
}

TSharedPtr<SWidget> FPopupRichTextDeco::CreateDecoratorWidget(const FTextRunInfo& RunInfo, const FTextBlockStyle& DefaultTextStyle) const
{
	FString PopupID;
	
	const FString* IdString = RunInfo.MetaData.Find(TEXT("id"));
	
	if (IdString)
	{
		PopupID = *IdString;
	}
	
	FText DisplayText = FText::FromString(RunInfo.Content.ToString());

	return SNew(SButton).OnClicked_Lambda([this, PopupID]()
	{
		if (Decorator)
		{
			Decorator->OpenPopup(Owner, PopupID);
		}
		return FReply::Handled();
	})
	[
		SNew(STextBlock).Text(DisplayText).TextStyle(&DefaultTextStyle)
	];
}



