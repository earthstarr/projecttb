// Fill out your copyright notice in the Description page of Project Settings.


#include "World/WorldPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PawnMovementComponent.h"

void AWorldPlayerController::ShowWidget(UUserWidget* Widget, bool bIgnoreMoveInput)
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
	
	if (bIgnoreMoveInput)
	{
		SetInputModeUIOnly();
	}

}

void AWorldPlayerController::CloseWidget(UUserWidget* Widget, bool bActiveMoveInput)
{
	if (Widget == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AWorldPlayerController ShowWidget함수 매개변수 Widget이 없습니다."));
		return;
	}

	Widget->RemoveFromParent();
	
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	
	if (bActiveMoveInput)
	{
		SetInputModeGameOnly();
	}

}

void AWorldPlayerController::SetInputModeGameOnly()
{
	//마우스 커서 제거
	bShowMouseCursor = false;
	
	//이동 다시 가능하도록 변경
	APawn* PlayerPawn = GetPawn();
	if (PlayerPawn == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AWorldPlayerController ShowWidget함수 Pawn이 없습니다."));
		return;
	}
	UMovementComponent* AMovementComponent = PlayerPawn->GetMovementComponent();
	if (AMovementComponent != nullptr)
	{
		SetIgnoreMoveInput(false);
	}
}

void AWorldPlayerController::SetInputModeUIOnly()
{
	//마우스 입력
	bShowMouseCursor = true;
	
	//기존 캐릭터의 이동은 중지, 추가 입력 제한
	APawn* PlayerPawn = GetPawn();
	if (PlayerPawn == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AWorldPlayerController ShowWidget함수 Pawn이 없습니다."));
		return;
	}
	UMovementComponent* AMovementComponent = PlayerPawn->GetMovementComponent();
	if (AMovementComponent != nullptr)
	{
		AMovementComponent->StopMovementImmediately();
		SetIgnoreMoveInput(true);
		PlayerPawn->GetMovementComponent()->Velocity = FVector::ZeroVector;
	}
}

