#include "UI/TBBattleHUD.h"

#include "TBGameInstance.h"
#include "UI/TurnOrderWidget.h"
#include "UI/BattleMenuWidget.h"
#include "UI/CharacterStatusWidget.h"
#include "UI/VictoryWidget.h"
#include "UI/LevelUpWidget.h"
#include "UI/PortalFloorWidget.h"
#include "UI/ScreenFadeBlockerWidget.h"
#include "Battle/BattleCombatant.h"
#include "Battle/BattlePlayerCharacter.h"
#include "Abilities/TBGameplayAbility.h"
#include "Kismet/GameplayStatics.h"
#include "World/PortalManager.h"

ATBBattleHUD::ATBBattleHUD() {}

void ATBBattleHUD::BeginPlay()
{
	Super::BeginPlay();
	
	//EnterBattleMode();
}

void ATBBattleHUD::EnterBattleMode()
{
	UE_LOG(LogTemp, Log, TEXT("ATBBattleHUD::EnterBattleMode Enter"));

	//위젯만 활성화 하고 배틀 매니저 바인딩은 포탈 매니저가 찾아서 SetBattleManager로 넘겨줌.
	CreateBattleWidgets();
}

void ATBBattleHUD::ExitBattleMode()
{
	UE_LOG(LogTemp, Log, TEXT("ATBBattleHUD::ExitBattleMode Enter"));
	
	if (BattleManager)
	{
		// 이벤트 연결 해제
		UnBindToBattleManager();
	}
	
	// 배틀 위젯 제거
	RemoveBattleWidgets();
	
	UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance());
	if (GI == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UPortalManager::SetBattleTransitionData GI is Nullptr"));
		return;
	}
	
	// 포탈 매니저에게 월드 맵으로 이동 요청
	UPortalManager* PortalManager = GetWorld()->GetSubsystem<UPortalManager>();
	PortalManager->OnReturnToWorldLevel(GI->BattleTransitionData.PostBattleRoomData);

	// 게임 인스턴스에서 BattleTransitionData 정보 초기화
	GI->BattleTransitionData = FBattleTransitionData();
}

void ATBBattleHUD::StartFadeOut(float Duration)
{
	
	if (APlayerController* PC = GetOwningPlayerController())
	{
		if (APlayerCameraManager* PlayerCameraManager = PC->PlayerCameraManager)
		{
			// 시작 투명도, 끝 투명도, 페이드 시간, 페이드 색상, 오디오 페이드 여부, 페이드 이후 상태 유지 여부
			PlayerCameraManager->StartCameraFade(0.f, 1.f, Duration, FLinearColor::Black, false, true);
		}
	}
}

void ATBBattleHUD::StartFadeIn(float Duration)
{
	HideScreenFadeBlocker();

	if (APlayerController* PC = GetOwningPlayerController())
	{
		if (APlayerCameraManager* PlayerCameraManager = PC->PlayerCameraManager)
		{
			PlayerCameraManager->StartCameraFade(
				1.f, 0.f, Duration, FLinearColor::Black, false, false);
		}
	}
}

void ATBBattleHUD::SetBattleManager(ABattleManager* NewBattleManager)
{
	if (!NewBattleManager)
	{
		return;
	}

	// 재탐색 이후 배틀 매니저 등록과 배틀 매니저의 등록이 중복되면 하나만 받도록 설정. 작업 환경이 다르기 때문에 양쪽 로직 살리는 쪽으로 설계
	if (BattleManager == NewBattleManager)
	{
		return;
	}

	// 배틀 매니저가 다른걸로 교체된다면 기존 매니저 제거. 현재 로직 흐름상 사용될 일은 없지만 남겨둠.
	if (BattleManager)
	{
		UnBindToBattleManager();
	}

	BattleManager = NewBattleManager;
	BindToBattleManager();
}

void ATBBattleHUD::CreateBattleWidgets()
{
	UE_LOG(LogTemp, Log, TEXT("ATBBattleHUD::CreateWidgets Enter"));

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("TBBattleHUD::CreateWidgets - PlayerController is null!"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("TBBattleHUD::CreateWidgets - TurnOrderWidgetClass: %s"),
		TurnOrderWidgetClass ? *TurnOrderWidgetClass->GetName() : TEXT("NULL"));

	if (TurnOrderWidgetClass)
	{
		TurnOrderWidget = CreateWidget<UTurnOrderWidget>(PC, TurnOrderWidgetClass);
		UE_LOG(LogTemp, Warning, TEXT("TBBattleHUD::CreateWidgets - TurnOrderWidget created: %s"),
			TurnOrderWidget ? TEXT("YES") : TEXT("NO"));
		if (TurnOrderWidget) TurnOrderWidget->AddToViewport(0);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("TBBattleHUD::CreateWidgets - TurnOrderWidgetClass is NOT set!"));
	}
	if (BattleMenuWidgetClass)
	{
		BattleMenuWidget = CreateWidget<UBattleMenuWidget>(PC, BattleMenuWidgetClass);
		if (BattleMenuWidget)
		{
			BattleMenuWidget->BattleManager = BattleManager;
			BattleMenuWidget->AddToViewport(1);
		}
	}
	if (CharacterStatusWidgetClass)
	{
		CharacterStatusWidget = CreateWidget<UCharacterStatusWidget>(PC, CharacterStatusWidgetClass);
		if (CharacterStatusWidget)
		{
			CharacterStatusWidget->AddToViewport(0);
			CharacterStatusWidget->SetVisibility(ESlateVisibility::Collapsed);  // 스탯 적용 전까지 완전히 숨김
		}
	}
}

void ATBBattleHUD::RemoveBattleWidgets()
{
	UE_LOG(LogTemp, Log, TEXT("ATBBattleHUD::RemoveBattleWidgets Enter"));

	if (TurnOrderWidget)
	{
		TurnOrderWidget->RemoveFromParent();
		TurnOrderWidget = nullptr;
	}
	if (BattleMenuWidget)
	{
		BattleMenuWidget->RemoveFromParent();
		BattleMenuWidget = nullptr;
	}
	if (CharacterStatusWidget)
	{
		CharacterStatusWidget->RemoveFromParent();
		CharacterStatusWidget = nullptr;
	}
}

void ATBBattleHUD::BindToBattleManager()
{
	UE_LOG(LogTemp, Log, TEXT("ATBBattleHUD::BindToBattleManager Enter"));

	if (!BattleManager) return;

	// CreateWidgets()는 BattleManager 탐색 전에 실행되므로 여기서 주입
	if (BattleMenuWidget)
		BattleMenuWidget->BattleManager = BattleManager;

	BattleManager->OnBattlePhaseChanged.AddDynamic(this, &ATBBattleHUD::HandlePhaseChanged);
	BattleManager->OnTurnBegin.AddDynamic(this,          &ATBBattleHUD::HandleTurnBegin);
	BattleManager->OnTurnOrderUpdated.AddDynamic(this,   &ATBBattleHUD::HandleTurnOrderUpdated);
	BattleManager->OnAnyDamageDealt.AddDynamic(this,     &ATBBattleHUD::HandleDamageDealt);
	BattleManager->OnAnyHealDealt.AddDynamic(this,       &ATBBattleHUD::HandleHealDealt);
	BattleManager->OnBattleReady.AddDynamic(this,        &ATBBattleHUD::HandleBattleReady);
	BattleManager->OnDiceRolled.AddDynamic(this,         &ATBBattleHUD::HandleDiceRolled);

	// CharacterStatusWidget 초기화는 OnBattleReady에서 수행 (스탯 적용 완료 후)

	// 배틀씬: UI 전용 입력 모드 (마우스 클릭으로 포커스 빠지는 것 방지)
	if (APlayerController* PC = GetOwningPlayerController())
	{
		FInputModeUIOnly InputMode;
		if (BattleMenuWidget)
			InputMode.SetWidgetToFocus(BattleMenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
		
		// 클릭 무시
		PC->bEnableClickEvents = false;
		PC->bEnableMouseOverEvents= false;
	}
}

void ATBBattleHUD::UnBindToBattleManager()
{
	UE_LOG(LogTemp, Log, TEXT("ATBBattleHUD::UnBindToBattleManager Enter"));

	if (!BattleManager) return;
	
	//델리게이트 제거

	BattleManager->OnBattlePhaseChanged.RemoveDynamic(this, &ATBBattleHUD::HandlePhaseChanged);
	BattleManager->OnTurnBegin.RemoveDynamic(this,          &ATBBattleHUD::HandleTurnBegin);
	BattleManager->OnTurnOrderUpdated.RemoveDynamic(this,   &ATBBattleHUD::HandleTurnOrderUpdated);
	BattleManager->OnAnyDamageDealt.RemoveDynamic(this,     &ATBBattleHUD::HandleDamageDealt);
	BattleManager->OnAnyHealDealt.RemoveDynamic(this,       &ATBBattleHUD::HandleHealDealt);
	BattleManager->OnBattleReady.RemoveDynamic(this,        &ATBBattleHUD::HandleBattleReady);
	BattleManager->OnDiceRolled.RemoveDynamic(this,         &ATBBattleHUD::HandleDiceRolled);
	
	// MP/Stamina 변경 구독
	for (ABattlePlayerCharacter* P : BattleManager->GetPlayerParty())
		if (P) P->OnStatChanged.RemoveDynamic(this, &ATBBattleHUD::HandleStatChanged);
	
	// 월드씬: UI 전용 입력 모드 해제
	if (APlayerController* PC = GetOwningPlayerController())
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
		
		// 클릭 가능하되 마우스 커서는 일단 안보이게
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents= true;
	}
	
	BattleManager = nullptr;
}

void ATBBattleHUD::HandlePhaseChanged(EBattlePhase NewPhase)
{
	UE_LOG(LogTemp, Log, TEXT("ATBBattleHUD::HandlePhaseChanged Enter"));

	if (!BattleManager) return;

	switch (NewPhase)
	{
	case EBattlePhase::BattleStart:
		// UI 초기화는 OnBattleReady에서 수행 (스탯 적용 완료 후)
		break;

	case EBattlePhase::PlayerTurn:
		if (BattleMenuWidget)
		{
			BattleMenuWidget->ShowMainMenu(BattleManager->GetCurrentActor());
			// 매 턴마다 입력 모드와 포커스 재설정 (카메라 전환 후 포커스 손실 방지)
			if (APlayerController* PC = GetOwningPlayerController())
			{
				FInputModeUIOnly InputMode;
				InputMode.SetWidgetToFocus(BattleMenuWidget->TakeWidget());
				InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
				PC->SetInputMode(InputMode);
				PC->bShowMouseCursor = false;
			}
		}
		// MP/Stamina 포함 전체 상태 갱신
		if (CharacterStatusWidget)
			for (ABattlePlayerCharacter* P : BattleManager->GetPlayerParty())
				CharacterStatusWidget->RefreshStatus(P);
		break;

	case EBattlePhase::SelectingTarget:
		if (BattleMenuWidget)
		{
			const EAbilityTargetType TargetType = BattleManager->GetPendingTargetType();
			switch (TargetType)
			{
			case EAbilityTargetType::SingleEnemy:
				BattleMenuWidget->ShowTargetSelection(BattleManager->GetLivingEnemies());
				break;
			case EAbilityTargetType::AllEnemies:
				BattleMenuWidget->ShowTargetSelectionAll(BattleManager->GetLivingEnemies());
				break;
			case EAbilityTargetType::SingleAlly:
				BattleMenuWidget->ShowTargetSelection(BattleManager->GetLivingPlayers());
				break;
			case EAbilityTargetType::AllAllies:
				BattleMenuWidget->ShowTargetSelectionAll(BattleManager->GetLivingPlayers());
				break;
			default:
				BattleMenuWidget->ShowTargetSelection(BattleManager->GetLivingEnemies());
				break;
			}
		}
		break;

	case EBattlePhase::DiceRolling:
		if (BattleMenuWidget)
			BattleMenuWidget->HideMenu();
		// OnDiceRolled 델리게이트 결과 오버레이는 Blueprint에서 처리
		break;

	case EBattlePhase::ExecutingAction:
		if (BattleMenuWidget)
			BattleMenuWidget->HideMenu();
		break;

	case EBattlePhase::EnemyTurn:
		if (BattleMenuWidget)
			BattleMenuWidget->HideMenu();
		// 플레이어 액션 완료 후 MP/Stamina 반영 (ExecutingAction → EnemyTurn 전환 시점)
		if (CharacterStatusWidget)
			for (ABattlePlayerCharacter* P : BattleManager->GetPlayerParty())
				CharacterStatusWidget->RefreshStatus(P);
		break;

	default:
		break;
	}
}

void ATBBattleHUD::HandleTurnBegin(ABattleCombatant* Combatant)
{
	UE_LOG(LogTemp, Log, TEXT("ATBBattleHUD::HandleTurnBegin Enter"));

	if (CharacterStatusWidget)
		CharacterStatusWidget->SetActiveCharacter(Combatant);

	// 카메라는 BattleManager의 고정 카메라 사용 — 턴별 전환 제거
}

void ATBBattleHUD::HandleTurnOrderUpdated(const TArray<ABattleCombatant*>& UpcomingTurns)
{
	UE_LOG(LogTemp, Warning, TEXT("HandleTurnOrderUpdated called - UpcomingTurns: %d, TurnOrderWidget valid: %s"),
		UpcomingTurns.Num(), TurnOrderWidget ? TEXT("YES") : TEXT("NO"));

	if (TurnOrderWidget)
		TurnOrderWidget->UpdateTurnOrder(UpcomingTurns);
}

void ATBBattleHUD::HandleDamageDealt(ABattleCombatant* Target, float /*Damage*/)
{
	if (CharacterStatusWidget && Target)
		CharacterStatusWidget->RefreshStatus(Target);
}

void ATBBattleHUD::HandleHealDealt(ABattleCombatant* Target, float /*Heal*/)
{
	if (CharacterStatusWidget && Target)
		CharacterStatusWidget->RefreshStatus(Target);
}

void ATBBattleHUD::HandleStatChanged(ABattleCombatant* Combatant)
{
	if (CharacterStatusWidget && Combatant)
		CharacterStatusWidget->RefreshStatus(Combatant);
}

void ATBBattleHUD::HandleBattleReady()
{
	UE_LOG(LogTemp, Log, TEXT("ATBBattleHUD::HandleBattleReady Enter"));

	if (!BattleManager) return;

	// 스탯 적용 완료 후 파티 UI 초기화
	if (CharacterStatusWidget)
	{
		CharacterStatusWidget->InitializeParty(BattleManager->GetPlayerParty());

		// 초기 바 갱신 (InitializeParty 직후 RefreshStatus 호출)
		for (ABattlePlayerCharacter* P : BattleManager->GetPlayerParty())
		{
			if (P) CharacterStatusWidget->RefreshStatus(P);
		}

		CharacterStatusWidget->SetVisibility(ESlateVisibility::Visible);  // 초기화 완료 후 표시
	}

	// MP/Stamina 변경 구독
	for (ABattlePlayerCharacter* P : BattleManager->GetPlayerParty())
		if (P) P->OnStatChanged.AddUniqueDynamic(this, &ATBBattleHUD::HandleStatChanged);
}

void ATBBattleHUD::HandleDiceRolled(int32 FaceValue, float Multiplier)
{
	GetWorldTimerManager().ClearTimer(DiceOverlayTimer);
	ShowDiceResultOverlay(FaceValue, Multiplier);
	GetWorldTimerManager().SetTimer(DiceOverlayTimer, this, &ATBBattleHUD::HideDiceResultOverlay, 2.0f, false);
}

// ─── Victory/LevelUp ──────────────────────────────────────────────────────────

void ATBBattleHUD::ShowVictoryWidget(const TArray<FCharacterExpData>& BeforeExpData,
                                      const TArray<FCharacterExpData>& AfterExpData,
                                      const TArray<FLevelUpDisplayData>& LevelUpData,
                                      int32 RewardMoney)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !VictoryWidgetClass) return;

	VictoryWidget = CreateWidget<UVictoryWidget>(PC, VictoryWidgetClass);
	if (VictoryWidget)
	{
		VictoryWidget->OwningHUD = this;
		VictoryWidget->Initialize(BeforeExpData, AfterExpData, LevelUpData, RewardMoney);
		VictoryWidget->AddToViewport(10);
	}
}

void ATBBattleHUD::ShowLevelUpWidget(const TArray<FLevelUpDisplayData>& LevelUpData)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !LevelUpWidgetClass) return;

	LevelUpWidget = CreateWidget<ULevelUpWidget>(PC, LevelUpWidgetClass);
	if (LevelUpWidget)
	{
		LevelUpWidget->OwningHUD = this;
		LevelUpWidget->Initialize(LevelUpData);
		LevelUpWidget->AddToViewport(10);
	}
}

void ATBBattleHUD::ReturnToWorld()
{
	ExitBattleMode();
}

void ATBBattleHUD::ShowMainMenu()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !MainMenuWidgetClass) return;

	if (!MainMenuWidget)
	{
		MainMenuWidget = CreateWidget<UUserWidget>(PC, MainMenuWidgetClass);
	}

	if (MainMenuWidget)
	{
		if (!MainMenuWidget->IsInViewport())
		{
			MainMenuWidget->AddToViewport();
		}
		MainMenuWidget->SetVisibility(ESlateVisibility::Visible);
	}

	StartFadeIn();
}

void ATBBattleHUD::ShowScreenFadeBlocker()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !ScreenFadeBlockerWidgetClass)
	{
		return;
	}

	if (!ScreenFadeBlockerWidget)
	{
		ScreenFadeBlockerWidget = CreateWidget<UScreenFadeBlockerWidget>(PC, ScreenFadeBlockerWidgetClass);
	}

	if (ScreenFadeBlockerWidget && !ScreenFadeBlockerWidget->IsInViewport())
	{
		ScreenFadeBlockerWidget->AddToViewport(200);
	}
}

void ATBBattleHUD::HideScreenFadeBlocker()
{
	if (!ScreenFadeBlockerWidget)
	{
		return;
	}

	UScreenFadeBlockerWidget* WidgetToHide = ScreenFadeBlockerWidget;
	ScreenFadeBlockerWidget = nullptr;

	if (WidgetToHide->IsInViewport())
	{
		WidgetToHide->PlayFadeOutAndRemove();
	}
}

void ATBBattleHUD::RemoveScreenFadeBlockerImmediately()
{
	if (!ScreenFadeBlockerWidget)
	{
		return;
	}

	UScreenFadeBlockerWidget* WidgetToRemove = ScreenFadeBlockerWidget;
	ScreenFadeBlockerWidget = nullptr;
	WidgetToRemove->RemoveImmediately();
}

void ATBBattleHUD::ShowPortalFloorWidget()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PortalFloorWidgetClass)
	{
		return;
	}

	if (!PortalFloorWidget)
	{
		PortalFloorWidget = CreateWidget<UPortalFloorWidget>(PC, PortalFloorWidgetClass);
	}

	if (PortalFloorWidget && !PortalFloorWidget->IsInViewport())
	{
		PortalFloorWidget->AddToViewport(5);
	}
}

void ATBBattleHUD::RemovePortalFloorWidget()
{
	if (!PortalFloorWidget)
	{
		return;
	}

	if (PortalFloorWidget->IsInViewport())
	{
		PortalFloorWidget->RemoveFromParent();
	}

	PortalFloorWidget = nullptr;
}

void ATBBattleHUD::ShowDefeatWidget()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !DefeatWidgetClass) return;

	// 배틀 위젯 제거 (입력 차단 방지)
	RemoveBattleWidgets();

	if (!DefeatWidget)
	{
		DefeatWidget = CreateWidget<UUserWidget>(PC, DefeatWidgetClass);
	}

	if (DefeatWidget && !DefeatWidget->IsInViewport())
	{
		DefeatWidget->AddToViewport(100);  // 높은 ZOrder로 최상위 표시
	}

	// UI 입력 모드로 전환
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(DefeatWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = true;

	// 클릭/호버 이벤트 활성화
	PC->bEnableClickEvents = true;
	PC->bEnableMouseOverEvents = true;
}

void ATBBattleHUD::ShowFinalVictoryWidget()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !FinalVictoryWidgetClass) return;

	// 배틀 위젯 제거
	RemoveBattleWidgets();

	if (!FinalVictoryWidget)
	{
		FinalVictoryWidget = CreateWidget<UUserWidget>(PC, FinalVictoryWidgetClass);
	}

	if (FinalVictoryWidget && !FinalVictoryWidget->IsInViewport())
	{
		FinalVictoryWidget->AddToViewport(100);  // 높은 ZOrder로 최상위 표시
	}

	// UI 입력 모드로 전환
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(FinalVictoryWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = true;

	// 클릭/호버 이벤트 활성화
	PC->bEnableClickEvents = true;
	PC->bEnableMouseOverEvents = true;
}

void ATBBattleHUD::ShowFinalVictorySecondWidget()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !FinalVictorySecondWidgetClass) return;

	// 첫 번째 최종 승리 위젯 제거
	if (FinalVictoryWidget && FinalVictoryWidget->IsInViewport())
	{
		FinalVictoryWidget->RemoveFromParent();
	}

	if (!FinalVictorySecondWidget)
	{
		FinalVictorySecondWidget = CreateWidget<UUserWidget>(PC, FinalVictorySecondWidgetClass);
	}

	if (FinalVictorySecondWidget && !FinalVictorySecondWidget->IsInViewport())
	{
		FinalVictorySecondWidget->AddToViewport(100);  // 높은 ZOrder로 최상위 표시
	}

	// UI 입력 모드로 전환
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(FinalVictorySecondWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = true;

	// 클릭/호버 이벤트 활성화
	PC->bEnableClickEvents = true;
	PC->bEnableMouseOverEvents = true;
}

void ATBBattleHUD::ReturnToMainMenu()
{
	RemovePortalFloorWidget();
	// 게임 일시정지 해제
	UGameplayStatics::SetGamePaused(GetWorld(), false);

	// 시간 감속 해제 (정상 속도로 복원)
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);

	// 패배 위젯 제거
	if (DefeatWidget)
	{
		if (DefeatWidget->IsInViewport())
		{
			DefeatWidget->RemoveFromParent();
		}
		DefeatWidget = nullptr;
	}

	// 최종 승리 위젯들도 제거
	if (FinalVictoryWidget && FinalVictoryWidget->IsInViewport())
	{
		FinalVictoryWidget->RemoveFromParent();
	}
	if (FinalVictorySecondWidget && FinalVictorySecondWidget->IsInViewport())
	{
		FinalVictorySecondWidget->RemoveFromParent();
	}

	// 페이드 아웃
	StartFadeOut(0.5f);

	// 0.5초 후 GameInstance의 ReturnToMainMenu 호출
	FTimerHandle ReturnTimer;
	GetWorldTimerManager().SetTimer(ReturnTimer, [this]()
	{
		if (UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance()))
		{
			GI->ReturnToMainMenu();
		}
	}, 1.0f, false);
}
