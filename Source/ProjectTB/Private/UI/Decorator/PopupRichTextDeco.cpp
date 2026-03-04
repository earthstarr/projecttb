// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Decorator/PopupRichTextDeco.h"

#include "Blueprint/UserWidget.h"

TSharedPtr<ITextDecorator> UPopupRichTextDeco::CreateDecorator(URichTextBlock* InOwner)
{
	return Super::CreateDecorator(InOwner);
	
	return MakeShareable(
	new UPopupRichTextDeco(InOwner, this)
	);
	
}
