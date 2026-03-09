#include "UI/TBBattleHUD.h"

#include "TBGameInstance.h"
#include "UI/TurnOrderWidget.h"
#include "UI/BattleMenuWidget.h"
#include "UI/CharacterStatusWidget.h"
#include "Battle/BattleCombatant.h"
#include "Battle/BattlePlayerCharacter.h"
#include "Abilities/TBGameplayAbility.h"
#include "Kismet/GameplayStatics.h"
#include "World/PotalManager.h"

ATBBattleHUD::ATBBattleHUD() {}

void ATBBattleHUD::BeginPlay()
{
	Super::BeginPlay();
	
	EnterBattleMode();
}

void ATBBattleHUD::EnterBattleMode()
{
	UE_LOG(LogTemp, Log, TEXT("ATBBattleHUD::EnterBattleMode Enter"));

	CreateBattleWidgets();

	// 레벨에서 BattleManager를 찾아 바인딩
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABattleManager::StaticClass(), Found);
	if (!Found.IsEmpty())
	{
		BattleManager = Cast<ABattleManager>(Found[0]);
		BindToBattleManager();
	}
}

void ATBBattleHUD::ExitBattleMode()
{
	UE_LOG(LogTemp, Log, TEXT("ATBBattleHUD::ExitBattleMode Enter"));
	
	// 이벤트 연결 해제
	UnBindToBattleManager();
	
	// 배틀 위젯 제거
	RemoveBattleWidgets();
	
	// 포탈 매니저에게 월드 맵으로 이동 요청
	UPotalManager* PotalManager = GetWorld()->GetSubsystem<UPotalManager>();
	if (UTBGameInstance* TBGameInstance = Cast<UTBGameInstance>(GetGameInstance()))
	{
		PotalManager->OnReturnToWorldLevel(TBGameInstance->BattleTransitionData.ReturnRoomData);
	}
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
