// Fill out your copyright notice in the Description page of Project Settings.


#include "World/WorldPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "UI/Artifact/OwnedArtifactListWidget.h"
#include "UI/World/ShopWidget.h"
#include "TBGameInstance.h"
#include "UI/LevelUpWidget.h"

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
	// Input Mode를 UI Only로 변경
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	bShowMouseCursor = true;

	// 캐릭터 이동 중지
	APawn* PlayerPawn = GetPawn();
	if (PlayerPawn)
	{
		UMovementComponent* MoveComp = PlayerPawn->GetMovementComponent();
		if (MoveComp)
		{
			MoveComp->StopMovementImmediately();
			MoveComp->Velocity = FVector::ZeroVector;
		}
		SetIgnoreMoveInput(true);
	}
}

void AWorldPlayerController::SetInputModeBattle()
{
	UE_LOG(LogTemp, Log, TEXT("AWorldPlayerController::SetInputModeBattle Enter"));
	
	
	// 이동 차단
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn == nullptr) return;
	if (ControlledPawn->IsHidden()) return;
	
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

	// Input Mode를 Game Only로 변경
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;

	// 이동 활성화
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn == nullptr) return;

	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
	
	// 카메라 활성화
	SetViewTarget(ControlledPawn);

	// 엑터가 보이지 않는다면 활성화
	//if (ControlledPawn->IsHidden() == false) return;

	// MoveComp->SetMovementMode(MOVE_Walking); // 이동 모드는 TeleportLevel에서 켜줌.

	// 액터 활성화
	ControlledPawn->SetActorHiddenInGame(false);
	ControlledPawn->SetActorEnableCollision(true);	

}

void AWorldPlayerController::ToggleWorldUIMode()
{
	// UI 모드 진입
	if (bWorldUIMode == false)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

		// 포커스를 줄 메인 UI가 있으면 지정
		// 예: OwnedArtifactListWidget->TakeWidget()

		SetInputMode(InputMode);
		bShowMouseCursor = true;
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);

		if (const APawn* WorldPawn = GetPawn())
		{
			if (UMovementComponent* MoveComp = WorldPawn->GetMovementComponent())
			{
				MoveComp->StopMovementImmediately();
			}
		}

		bWorldUIMode = true;
	}
	// UI 모드 해제
	else
	{
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
		SetIgnoreMoveInput(false);
		SetIgnoreLookInput(false);
		bWorldUIMode = false;
	}
	
}

void AWorldPlayerController::TogglePartyStatus()
{
	// 다른 전용 UI(상점/이벤트 등)가 열려 있으면 상태창은 열지 않음
	if (CurrentWidget && CurrentWidget != PartyStatusWidget)
	{
		return;
	}

	if (PartyStatusWidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AWorldPlayerController::TogglePartyStatus - PartyStatusWidgetClass is nullptr"));
		return;
	}

	if (PartyStatusWidget == nullptr)
	{
		PartyStatusWidget = CreateWidget<ULevelUpWidget>(this, PartyStatusWidgetClass);
		if (PartyStatusWidget == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("AWorldPlayerController::TogglePartyStatus - Failed to create PartyStatusWidget"));
			return;
		}
	}

	const bool bIsOpen = PartyStatusWidget->IsInViewport();

	// 열기
	if (!bIsOpen)
	{
		PartyStatusWidget->AddToViewport(50);
		PartyStatusWidget->RefreshFromGameState();

		CurrentWidget = PartyStatusWidget;

		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);

		bShowMouseCursor = true;
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);

		if (APawn* PlayerPawn = GetPawn())
		{
			if (UMovementComponent* MoveComp = PlayerPawn->GetMovementComponent())
			{
				MoveComp->StopMovementImmediately();
				MoveComp->Velocity = FVector::ZeroVector;
			}
		}

		return;
	}

	// 닫기
	PartyStatusWidget->RemoveFromParent();

	if (CurrentWidget == PartyStatusWidget)
	{
		CurrentWidget = nullptr;
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	bShowMouseCursor = false;
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
}

void AWorldPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindKey(EKeys::Eight, IE_Pressed, this, &AWorldPlayerController::CheatSetAllLevel9);
	InputComponent->BindKey(EKeys::Nine,  IE_Pressed, this, &AWorldPlayerController::CheatAddGold10000);
	InputComponent->BindKey(EKeys::Zero,  IE_Pressed, this, &AWorldPlayerController::CheatSetPortalCount13);
}

void AWorldPlayerController::CheatSetAllLevel9()
{
	UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance());
	if (!GI) return;

	for (FPartyMemberData& Member : GI->PartyData)
	{
		Member.Level = 9;
		Member.CurrentExp = 0;
	}
	UE_LOG(LogTemp, Warning, TEXT("[Cheat] All party members set to Level 9"));
}

void AWorldPlayerController::CheatAddGold10000()
{
	UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance());
	if (!GI) return;

	GI->AddMoney(10000);
	UE_LOG(LogTemp, Warning, TEXT("[Cheat] Added 10000 gold. Total: %d"), GI->GetCurrentMoney());
}

void AWorldPlayerController::CheatSetPortalCount13()
{
	UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance());
	if (!GI) return;

	GI->SetPortalMoveCount(13);
	UE_LOG(LogTemp, Warning, TEXT("[Cheat] PortalMoveCount set to 13"));
}

void AWorldPlayerController::PortalSetCount(int32 NewCount)
{
	if (UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance()))
	{
		GI->SetPortalMoveCount(NewCount);
		UE_LOG(LogTemp, Warning, TEXT("[PortalCheat] PortalMoveCount set to %d"), GI->GetPortalMoveCount());
	}
}

void AWorldPlayerController::PortalAddCount(int32 Delta)
{
	if (UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance()))
	{
		GI->SetPortalMoveCount(GI->GetPortalMoveCount() + Delta);
		UE_LOG(LogTemp, Warning, TEXT("[PortalCheat] PortalMoveCount changed to %d"), GI->GetPortalMoveCount());
	}
}

void AWorldPlayerController::PortalPrintCount() const
{
	if (const UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalCheat] PortalMoveCount = %d"), GI->GetPortalMoveCount());
	}
}