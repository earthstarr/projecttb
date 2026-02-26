#include "Battle/BattleManager.h"
#include "Battle/BattlePlayerCharacter.h"
#include "Battle/BattleEnemyCharacter.h"
#include "Abilities/TBGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraActor.h"
#include "GameFramework/PlayerController.h"

ABattleManager::ABattleManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABattleManager::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStartBattle)
	{
		// 0.5초 딜레이 — 레벨의 모든 Actor BeginPlay 완료 후 실행
		FTimerHandle AutoStartTimer;
		GetWorldTimerManager().SetTimer(AutoStartTimer, this, &ABattleManager::AutoStartBattle, 0.5f, false);
	}
}

void ABattleManager::AutoStartBattle()
{
	TArray<AActor*> FoundPlayers;
	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABattlePlayerCharacter::StaticClass(), FoundPlayers);
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABattleEnemyCharacter::StaticClass(),  FoundEnemies);

	TArray<ABattlePlayerCharacter*> Players;
	TArray<ABattleEnemyCharacter*>  Enemies;

	for (AActor* A : FoundPlayers)
		if (ABattlePlayerCharacter* P = Cast<ABattlePlayerCharacter>(A)) Players.Add(P);
	for (AActor* A : FoundEnemies)
		if (ABattleEnemyCharacter* E = Cast<ABattleEnemyCharacter>(A))  Enemies.Add(E);

	if (Players.IsEmpty() || Enemies.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("BattleManager: 레벨에 Player(%d) 또는 Enemy(%d)가 없어 전투를 시작할 수 없습니다."),
			Players.Num(), Enemies.Num());
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("BattleManager: AutoStartBattle → Player %d명, Enemy %d명"), Players.Num(), Enemies.Num());
	StartBattle(Players, Enemies);
}

// ─── 전투 시작 ─────────────────────────────────────────────────────────────────
void ABattleManager::StartBattle(
	const TArray<ABattlePlayerCharacter*>& Players,
	const TArray<ABattleEnemyCharacter*>&  Enemies)
{
	PlayerParty.Reset();
	EnemyParty.Reset();

	for (ABattlePlayerCharacter* P : Players)
	{
		if (!P || P->IsDead()) continue;
		PlayerParty.Add(P);
		P->OnDeath.AddDynamic(this, &ABattleManager::OnCombatantDied);
		P->OnDamageReceived.AddDynamic(this, &ABattleManager::OnCombatantDamaged);
	}
	for (ABattleEnemyCharacter* E : Enemies)
	{
		if (!E || E->IsDead()) continue;
		EnemyParty.Add(E);
		E->OnDeath.AddDynamic(this, &ABattleManager::OnCombatantDied);
		E->OnDamageReceived.AddDynamic(this, &ABattleManager::OnCombatantDamaged);
	}

	BuildRoundOrder();
	SetPhase(EBattlePhase::BattleStart);

	// 고정 카메라로 전환
	if (BattleCamera)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			PC->SetViewTargetWithBlend(BattleCamera, 0.5f);
		}
	}

	// 1초 후 첫 턴 시작 (BattleStart 연출 시간)
	FTimerHandle StartTimer;
	GetWorldTimerManager().SetTimer(StartTimer, this, &ABattleManager::AdvanceTurn, 1.f, false);
}

// ─── 턴 순서 구성 (Speed 내림차순) ─────────────────────────────────────────────
void ABattleManager::BuildRoundOrder()
{
	CurrentRoundOrder.Reset();

	for (ABattleCombatant* P : PlayerParty) if (P && !P->IsDead()) CurrentRoundOrder.Add(P);
	for (ABattleCombatant* E : EnemyParty)  if (E && !E->IsDead()) CurrentRoundOrder.Add(E);

	CurrentRoundOrder.Sort([](const ABattleCombatant& A, const ABattleCombatant& B)
	{
		return A.GetSpeed() > B.GetSpeed();
	});

	CurrentRoundIndex = 0;
	BroadcastTurnOrder();
}

// ─── 다음 턴으로 이동 ────────────────────────────────────────────────────────────
void ABattleManager::AdvanceTurn()
{
	CheckBattleEnd();
	if (CurrentPhase == EBattlePhase::BattleVictory ||
	    CurrentPhase == EBattlePhase::BattleDefeat) return;

	// 사망한 유닛 스킵
	while (CurrentRoundIndex < CurrentRoundOrder.Num() &&
	       (!CurrentRoundOrder[CurrentRoundIndex] || CurrentRoundOrder[CurrentRoundIndex]->IsDead()))
	{
		CurrentRoundIndex++;
	}

	// 라운드 종료 → 재구성
	if (CurrentRoundIndex >= CurrentRoundOrder.Num())
	{
		BuildRoundOrder();
		AdvanceTurn();
		return;
	}

	ABattleCombatant* Current = CurrentRoundOrder[CurrentRoundIndex];
	CurrentRoundIndex++;

	BroadcastTurnOrder();
	OnTurnBegin.Broadcast(Current);
	Current->OnTurnBegin();

	if (Cast<ABattlePlayerCharacter>(Current))
		StartPlayerTurn();
	else if (Cast<ABattleEnemyCharacter>(Current))
		StartEnemyTurn();
}

// ─── 플레이어 턴 ──────────────────────────────────────────────────────────────
void ABattleManager::StartPlayerTurn()
{
	PendingTarget       = nullptr;
	PendingAbilityClass = nullptr;
	SetPhase(EBattlePhase::PlayerTurn);
	UE_LOG(LogTemp, Display, TEXT("BattleManager: [PlayerTurn] %s"), GetCurrentActor() ? *GetCurrentActor()->GetName() : TEXT("None"));
}

// ─── 적 턴 ────────────────────────────────────────────────────────────────────
void ABattleManager::StartEnemyTurn()
{
	SetPhase(EBattlePhase::EnemyTurn);
	UE_LOG(LogTemp, Display, TEXT("BattleManager: [EnemyTurn] %s"), GetCurrentActor() ? *GetCurrentActor()->GetName() : TEXT("None"));

	ABattleEnemyCharacter* Enemy = Cast<ABattleEnemyCharacter>(GetCurrentActor());
	if (!Enemy) { OnActionComplete(); return; }

	ABattleCombatant* Target = nullptr;
	TSubclassOf<UTBGameplayAbility> AbilityClass;
	Enemy->SelectAction(GetLivingPlayers(), Target, AbilityClass);

	if (!Target || !AbilityClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("BattleManager: EnemyTurn - Target(%s) 또는 AbilityClass 없음, 턴 스킵"),
			Target ? *Target->GetName() : TEXT("null"));
		OnActionComplete(); return;
	}

	// 시각적 딜레이 후 실행
	PendingTarget       = Target;
	PendingAbilityClass = AbilityClass;
	GetWorldTimerManager().SetTimer(EnemyActionTimer, this,
		&ABattleManager::ExecuteEnemyActionDelayed, 0.5f, false);
}

void ABattleManager::ExecuteEnemyActionDelayed()
{
	ABattleEnemyCharacter* Enemy = Cast<ABattleEnemyCharacter>(GetCurrentActor());
	if (Enemy && PendingTarget && PendingAbilityClass)
		ExecuteAction(Enemy, PendingTarget, PendingAbilityClass);
	else
		OnActionComplete();
}

// ─── 플레이어 입력 처리 ──────────────────────────────────────────────────────────
void ABattleManager::PlayerSelectAttack()
{
	UE_LOG(LogTemp, Display, TEXT("BattleManager: PlayerSelectAttack called, Phase=%d"), (int32)CurrentPhase);
	if (CurrentPhase != EBattlePhase::PlayerTurn) return;
	PendingAbilityClass = nullptr;
	UE_LOG(LogTemp, Display, TEXT("BattleManager: SetPhase SelectingTarget, Enemies=%d"), GetLivingEnemies().Num());
	SetPhase(EBattlePhase::SelectingTarget);
}

void ABattleManager::PlayerSelectAbility(TSubclassOf<UTBGameplayAbility> AbilityClass)
{
	if (CurrentPhase != EBattlePhase::PlayerTurn) return;
	PendingAbilityClass = AbilityClass;
	SetPhase(EBattlePhase::SelectingTarget);
}

void ABattleManager::PlayerSelectTarget(ABattleCombatant* Target)
{
	if (CurrentPhase != EBattlePhase::SelectingTarget || !Target) return;

	ABattleCombatant* Caster = GetCurrentActor();
	if (!Caster) return;

	// 어빌리티가 없으면 첫 번째 어빌리티(기본 공격)를 사용
	TSubclassOf<UTBGameplayAbility> AbilityToUse = PendingAbilityClass;
	if (!AbilityToUse)
	{
		TArray<UTBGameplayAbility*> Abilities = Caster->GetGrantedAbilities();
		if (!Abilities.IsEmpty())
			AbilityToUse = Abilities[0]->GetClass();
	}

	if (AbilityToUse)
		ExecuteAction(Caster, Target, AbilityToUse);
}

void ABattleManager::PlayerCancel()
{
	if (CurrentPhase == EBattlePhase::SelectingTarget)
	{
		PendingAbilityClass = nullptr;
		PendingTarget       = nullptr;
		SetPhase(EBattlePhase::PlayerTurn);
	}
}

// ─── 어빌리티 실행 ───────────────────────────────────────────────────────────────
void ABattleManager::ExecuteAction(
	ABattleCombatant* Caster,
	ABattleCombatant* Target,
	TSubclassOf<UTBGameplayAbility> AbilityClass)
{
	if (!Caster || !AbilityClass) { OnActionComplete(); return; }

	SetPhase(EBattlePhase::ExecutingAction);
	PendingTarget = Target; // 어빌리티가 GetPendingTarget()으로 읽기

	// 어빌리티 CDO에서 카메라 설정을 읽어 액션 카메라 전환
	if (UTBGameplayAbility* CDO = AbilityClass->GetDefaultObject<UTBGameplayAbility>())
	{
		if (CDO->bUseActionCamera)
			SwitchToActionCamera(Caster, CDO);
	}

	UAbilitySystemComponent* ASC = Caster->GetAbilitySystemComponent();
	if (!ASC) { OnActionComplete(); return; }

	// ASC에서 해당 클래스의 Spec을 찾아 활성화
	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(AbilityClass);
	if (Spec)
	{
		if (!ASC->TryActivateAbility(Spec->Handle))
		{
			// 비용 부족 등으로 활성화 실패 → 플레이어 턴으로 복귀
			if (Cast<ABattlePlayerCharacter>(Caster))
				StartPlayerTurn();
			else
				OnActionComplete();
		}
	}
	else
		OnActionComplete();
}

// ─── 액션 완료 (어빌리티가 EndAbility 후 호출) ────────────────────────────────
void ABattleManager::OnActionComplete()
{
	ReturnToBattleCamera();

	if (ABattleCombatant* Current = GetCurrentActor())
		Current->OnTurnEnd();

	CheckBattleEnd();

	if (CurrentPhase != EBattlePhase::BattleVictory &&
	    CurrentPhase != EBattlePhase::BattleDefeat)
	{
		FTimerHandle NextTurnTimer;
		GetWorldTimerManager().SetTimer(NextTurnTimer, this, &ABattleManager::AdvanceTurn, 0.5f, false);
	}
}

// ─── 쿼리 ─────────────────────────────────────────────────────────────────────
ABattleCombatant* ABattleManager::GetCurrentActor() const
{
	const int32 Index = CurrentRoundIndex - 1;
	return CurrentRoundOrder.IsValidIndex(Index) ? CurrentRoundOrder[Index] : nullptr;
}

TArray<ABattleCombatant*> ABattleManager::GetUpcomingTurns(int32 Count) const
{
	TArray<ABattleCombatant*> Result;
	const int32 RoundSize = CurrentRoundOrder.Num();
	if (RoundSize == 0) return Result;

	// 현재 액터부터 시작해 다음 라운드까지 포함해서 Count개 반환
	const int32 StartIndex = FMath::Max(CurrentRoundIndex - 1, 0);

	for (int32 Offset = 0; Result.Num() < Count; Offset++)
	{
		// 현재 라운드 → 다음 라운드 순환
		const int32 Idx = (StartIndex + Offset) % RoundSize;
		if (ABattleCombatant* C = CurrentRoundOrder[Idx])
			if (!C->IsDead()) Result.Add(C);

		if (Offset >= Count * 3) break; // 무한루프 방지
	}
	return Result;
}

TArray<ABattleCombatant*> ABattleManager::GetLivingPlayers() const
{
	TArray<ABattleCombatant*> Result;
	for (ABattleCombatant* P : PlayerParty) if (P && !P->IsDead()) Result.Add(P);
	return Result;
}

TArray<ABattleCombatant*> ABattleManager::GetLivingEnemies() const
{
	TArray<ABattleCombatant*> Result;
	for (ABattleCombatant* E : EnemyParty) if (E && !E->IsDead()) Result.Add(E);
	return Result;
}

TArray<ABattlePlayerCharacter*> ABattleManager::GetPlayerParty() const
{
	return PlayerParty;
}

// ─── 내부 유틸 ───────────────────────────────────────────────────────────────────
void ABattleManager::SetPhase(EBattlePhase NewPhase)
{
	CurrentPhase = NewPhase;
	OnBattlePhaseChanged.Broadcast(NewPhase);
}

void ABattleManager::CheckBattleEnd()
{
	if (GetLivingEnemies().IsEmpty())
		SetPhase(EBattlePhase::BattleVictory);
	else if (GetLivingPlayers().IsEmpty())
		SetPhase(EBattlePhase::BattleDefeat);
}

void ABattleManager::BroadcastTurnOrder()
{
	OnTurnOrderUpdated.Broadcast(GetUpcomingTurns(5));
}

void ABattleManager::OnCombatantDied(ABattleCombatant* Combatant)
{
	BroadcastTurnOrder();
	CheckBattleEnd();
}

void ABattleManager::OnCombatantDamaged(ABattleCombatant* Combatant, float Damage)
{
	OnAnyDamageDealt.Broadcast(Combatant, Damage);
}

// ─── 카메라 전환 ──────────────────────────────────────────────────────────────
void ABattleManager::SwitchToActionCamera(ABattleCombatant* Caster, const UTBGameplayAbility* AbilityCDO)
{
	if (!ActionCamera || !Caster || !AbilityCDO) return;

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	// 캐릭터 로컬 오프셋 → 월드 좌표로 변환
	const FTransform CasterTransform = Caster->GetActorTransform();
	const FVector    WorldLocation   = CasterTransform.TransformPosition(AbilityCDO->ActionCameraLocalOffset);
	const FRotator   WorldRotation   = CasterTransform.TransformRotation(
		AbilityCDO->ActionCameraLocalRotation.Quaternion()).Rotator();

	ActionCamera->SetActorLocation(WorldLocation);
	ActionCamera->SetActorRotation(WorldRotation);

	// 캐릭터에 Attach — Melee처럼 캐릭터가 이동해도 카메라가 따라감
	ActionCamera->AttachToActor(Caster, FAttachmentTransformRules::KeepWorldTransform);

	PendingCameraBlendOutTime = AbilityCDO->CameraBlendOutTime;
	bActionCameraActive = true;

	PC->SetViewTargetWithBlend(ActionCamera, AbilityCDO->CameraBlendInTime, VTBlend_Cubic);
}

void ABattleManager::ReturnToBattleCamera()
{
	if (!bActionCameraActive || !BattleCamera) return;
	bActionCameraActive = false;

	if (ActionCamera)
		ActionCamera->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
		PC->SetViewTargetWithBlend(BattleCamera, PendingCameraBlendOutTime, VTBlend_Cubic);
}
