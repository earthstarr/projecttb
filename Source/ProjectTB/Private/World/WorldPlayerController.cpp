// Fill out your copyright notice in the Description page of Project Settings.


#include "World/WorldPlayerController.h"

#include "Blueprint/UserWidget.h"

void AWorldPlayerController::ShowWidget(UUserWidget* Widget)
{
	if (Widget == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AWorldPlayerController ShowWidget함수 매개변수 Widget이 없습니다."));
		return;
	}

	Widget->AddToViewport();
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Widget->TakeWidget());
	SetInputMode(InputMode);
	
	//마우스 입력 (키보드 입력으로 변경해줄 것)
	bShowMouseCursor = true;
}
