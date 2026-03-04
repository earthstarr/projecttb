// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Decorator/PopupRichTextDeco.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Blueprint/UserWidget.h"
#include "Components/RichTextBlock.h"

TSharedPtr<ITextDecorator> UPopupRichTextDeco::CreateDecorator(URichTextBlock* InOwner)
{
	return MakeShareable(new FPopupRichTextDeco(InOwner, this));
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
		return OnPopupClicked(PopupID);
	})
		[
			SNew(STextBlock).Text(DisplayText).TextStyle(&DefaultTextStyle)
		];
}

FReply FPopupRichTextDeco::OnPopupClicked(FString PopupID) const
{
	if (!Decorator || !Decorator->PopupWidgetClass)
	{
		return FReply::Handled();
	}
	
	//이게 읽기 편한가? 다른 사람도 괜찮다면 이 방식도 괜찮아 보인다.
	UWorld* World = Owner ? Owner->GetWorld() : nullptr;

	if (World == nullptr)
	{
		return FReply::Handled();
	}

	UUserWidget* PopupWidget =
		CreateWidget<UUserWidget>(World, Decorator->PopupWidgetClass);

	if (PopupWidget)
	{
		PopupWidget->AddToViewport();
		
		if (PopupWidget->GetClass()->ImplementsInterface(UPopupRichTextDeco::StaticClass()))
		{
			SetPopupId(PopupID);
		}
		
	}

	//이벤트 처리 완료
	return FReply::Handled();
}



