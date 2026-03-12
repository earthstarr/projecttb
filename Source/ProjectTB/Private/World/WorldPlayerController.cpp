// Fill out your copyright notice in the Description page of Project Settings.


#include "World/WorldPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "UI/Artifact/OwnedArtifactListWidget.h"
#include "UI/World/ShopWidget.h"

void AWorldPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (OwnedArtifactListWidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ArtifactListDebug] OwnedArtifactListWidgetClass is nullptr in AWorldPlayerController::BeginPlay"));
		return;
	}

	OwnedArtifactListWidget = CreateWidget<UOwnedArtifactListWidget>(this, OwnedArtifactListWidgetClass);
	if (OwnedArtifactListWidget == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ArtifactListDebug] Failed to create OwnedArtifactListWidget in AWorldPlayerController::BeginPlay"));
		return;
	}

	OwnedArtifactListWidget->AddToViewport(10);
	OwnedArtifactListWidget->RestoreFieldLayout();
	UE_LOG(LogTemp, Warning, TEXT("[ArtifactListDebug] OwnedArtifactListWidget created and added to viewport in AWorldPlayerController::BeginPlay"));
}

void AWorldPlayerController::ShowWidget(UUserWidget* Widget, bool bIgnoreMoveInput)
{
	if (Widget == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AWorldPlayerController ShowWidget함수 매개변수 Widget이 없습니다."));
		return;
	}

	if (Widget->IsInViewport() == false)
	{
		Widget->AddToViewport();
	}

	if (UShopWidget* ShopWidget = Cast<UShopWidget>(Widget))
	{
		ShopWidget->ShowShopWidget();
		UE_LOG(LogTemp, Warning, TEXT("[ArtifactListDebug] Showing ShopWidget in AWorldPlayerController::ShowWidget"));

		if (OwnedArtifactListWidget)
		{
			OwnedArtifactListWidget->SetVisibility(ESlateVisibility::Collapsed);
			UE_LOG(LogTemp, Warning, TEXT("[ArtifactListDebug] Hid field OwnedArtifactListWidget in AWorldPlayerController::ShowWidget"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ArtifactListDebug] OwnedArtifactListWidget is nullptr while showing ShopWidget"));
		}
	}
	else
	{
		Widget->SetVisibility(ESlateVisibility::Visible);
	}
	
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(Widget->TakeWidget());
	SetInputMode(InputMode);

	CurrentWidget = Widget;
	
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

	if (UShopWidget* ShopWidget = Cast<UShopWidget>(Widget))
	{
		ShopWidget->HideShopWidget();
		UE_LOG(LogTemp, Warning, TEXT("[ArtifactListDebug] Hiding ShopWidget in AWorldPlayerController::CloseWidget"));

		if (OwnedArtifactListWidget)
		{
			OwnedArtifactListWidget->SetVisibility(ESlateVisibility::Visible);
			UE_LOG(LogTemp, Warning, TEXT("[ArtifactListDebug] Showed field OwnedArtifactListWidget in AWorldPlayerController::CloseWidget"));
		}
	}
	else
	{
		Widget->RemoveFromParent();
	}
	
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	if (CurrentWidget == Widget)
	{
		CurrentWidget = nullptr;
	}
	
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

void AWorldPlayerController::SetInputModeBattle()
{
	UE_LOG(LogTemp, Log, TEXT("AWorldPlayerController::SetInputModeBattle Enter"));

	// 이동 차단
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn == nullptr) return;
	
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	
	if (ACharacter* MyCharacter = Cast<ACharacter>(ControlledPawn))
	{
		if (UCharacterMovementComponent* MoveComp = MyCharacter->GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately(); // 이동 즉시 정지
			MoveComp->SetMovementMode(MOVE_None); // 이동 모드 해제
		}
	}

	// 액터 비활성화
	ControlledPawn->SetActorHiddenInGame(true);    
	ControlledPawn->SetActorEnableCollision(false);
	
	// UI 전용 입력 모드로 변경하는건 TBBattleHUD에서 BindToBattleManager가 처리
}

void AWorldPlayerController::SetInputModeWorld()
{
	UE_LOG(LogTemp, Log, TEXT("AWorldPlayerController::SetInputModeWorld Enter"));

	// 이동 활성화
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn == nullptr) return;
	
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	
	//		MoveComp->SetMovementMode(MOVE_Walking); // 이동 모드는 TeleportLevel에서 켜줌.

	// 액터 활성화
	ControlledPawn->SetActorHiddenInGame(false);    
	ControlledPawn->SetActorEnableCollision(true);
}

