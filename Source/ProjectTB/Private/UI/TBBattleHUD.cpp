#include "UI/TBBattleHUD.h"
#include "UI/TurnOrderWidget.h"
#include "UI/BattleMenuWidget.h"
#include "UI/CharacterStatusWidget.h"
#include "Battle/BattleCombatant.h"
#include "Battle/BattlePlayerCharacter.h"
#include "Kismet/GameplayStatics.h"

ATBBattleHUD::ATBBattleHUD() {}

void ATBBattleHUD::BeginPlay()
{
	Super::BeginPlay();
	CreateWidgets();

	// 레벨에서 BattleManager를 찾아 바인딩
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABattleManager::StaticClass(), Found);
	if (!Found.IsEmpty())
	{
		BattleManager = Cast<ABattleManager>(Found[0]);
		BindToBattleManager();
	}
}

void ATBBattleHUD::CreateWidgets()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	if (TurnOrderWidgetClass)
	{
		TurnOrderWidget = CreateWidget<UTurnOrderWidget>(PC, TurnOrderWidgetClass);
		if (TurnOrderWidget) TurnOrderWidget->AddToViewport(0);
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
		if (CharacterStatusWidget) CharacterStatusWidget->AddToViewport(0);
	}
}

void ATBBattleHUD::BindToBattleManager()
{
	if (!BattleManager) return;

	// CreateWidgets()는 BattleManager 탐색 전에 실행되므로 여기서 주입
	if (BattleMenuWidget)
		BattleMenuWidget->BattleManager = BattleManager;

	BattleManager->OnBattlePhaseChanged.AddDynamic(this, &ATBBattleHUD::HandlePhaseChanged);
	BattleManager->OnTurnBegin.AddDynamic(this,          &ATBBattleHUD::HandleTurnBegin);
	BattleManager->OnTurnOrderUpdated.AddDynamic(this,   &ATBBattleHUD::HandleTurnOrderUpdated);
	BattleManager->OnAnyDamageDealt.AddDynamic(this,     &ATBBattleHUD::HandleDamageDealt);

	// CharacterStatusWidget에 파티 정보 전달
	if (CharacterStatusWidget)
		CharacterStatusWidget->InitializeParty(BattleManager->GetPlayerParty());

	// 배틀씬: 마우스 잠금, 커서 숨김, UI 키보드 입력만 허용
	if (APlayerController* PC = GetOwningPlayerController())
	{
		FInputModeGameAndUI InputMode;
		if (BattleMenuWidget)
			InputMode.SetWidgetToFocus(BattleMenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}
}

void ATBBattleHUD::HandlePhaseChanged(EBattlePhase NewPhase)
{
	if (!BattleManager) return;

	switch (NewPhase)
	{
	case EBattlePhase::BattleStart:
		// AutoStartBattle 딜레이 이후 PlayerParty가 채워지므로 여기서 초기화
		if (CharacterStatusWidget)
			CharacterStatusWidget->InitializeParty(BattleManager->GetPlayerParty());
		break;

	case EBattlePhase::PlayerTurn:
		if (BattleMenuWidget)
		{
			BattleMenuWidget->ShowMainMenu(BattleManager->GetCurrentActor());
			BattleMenuWidget->SetUserFocus(GetOwningPlayerController());
		}
		// MP/Stamina 포함 전체 상태 갱신
		if (CharacterStatusWidget)
			for (ABattlePlayerCharacter* P : BattleManager->GetPlayerParty())
				CharacterStatusWidget->RefreshStatus(P);
		break;

	case EBattlePhase::SelectingTarget:
		if (BattleMenuWidget)
			BattleMenuWidget->ShowTargetSelection(BattleManager->GetLivingEnemies());
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
	if (CharacterStatusWidget)
		CharacterStatusWidget->SetActiveCharacter(Combatant);

	if (Combatant)
		if (APlayerController* PC = GetOwningPlayerController())
			PC->SetViewTargetWithBlend(Combatant, 0.5f, VTBlend_Cubic);
}

void ATBBattleHUD::HandleTurnOrderUpdated(const TArray<ABattleCombatant*>& UpcomingTurns)
{
	if (TurnOrderWidget)
		TurnOrderWidget->UpdateTurnOrder(UpcomingTurns);
}

void ATBBattleHUD::HandleDamageDealt(ABattleCombatant* Target, float /*Damage*/)
{
	if (CharacterStatusWidget && Target)
		CharacterStatusWidget->RefreshStatus(Target);
}
