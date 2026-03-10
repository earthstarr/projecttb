#include "Battle/BattleManager.h"
#include "TBGameplayTags.h"
#include "Battle/BattlePlayerCharacter.h"
#include "Battle/BattleEnemyCharacter.h"
#include "Battle/BattleImpactActor.h"
#include "Abilities/TBGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraActor.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraFunctionLibrary.h"
#include "TBGameInstance.h"
#include "Data/LevelDataTypes.h"
#include "UI/TBBattleHUD.h"
#include "World/PotalManager.h"

ABattleManager::ABattleManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ABattleManager::BeginPlay()
{
	Super::BeginPlay();
	
	// 0.5초 딜레이 — 레벨의 모든 Actor BeginPlay 완료 후 실행
	FTimerHandle StartTimer;
	if (bAutoStartBattle)
	{
		// 테스트용: 맵에 배치된 캐릭터 자동 감지
		GetWorldTimerManager().SetTimer(StartTimer, this, &ABattleManager::AutoStartBattle, 0.5f, false);
	}
	else
	{
		// 정식: GameInstance에서 데이터 읽어서 스폰
		GetWorldTimerManager().SetTimer(StartTimer, this, &ABattleManager::SpawnAndStartBattle, 0.5f, false);
	}
}

void ABattleManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 컷씬 카메라 부드러운 이동
	if (bCutsceneCameraBlending && ActionCamera)
	{
		CutsceneCameraBlendElapsed += DeltaTime;
		const float Alpha = FMath::Clamp(CutsceneCameraBlendElapsed / CutsceneCameraBlendTime, 0.f, 1.f);

		// Cubic ease-out 보간
		const float EasedAlpha = 1.f - FMath::Pow(1.f - Alpha, 3.f);

		const FVector NewLocation = FMath::Lerp(CutsceneCameraStartLocation, CutsceneCameraTargetLocation, EasedAlpha);
		const FRotator NewRotation = FMath::Lerp(CutsceneCameraStartRotation, CutsceneCameraTargetRotation, EasedAlpha);

		ActionCamera->SetActorLocation(NewLocation);
		ActionCamera->SetActorRotation(NewRotation);

		if (Alpha >= 1.f)
		{
			bCutsceneCameraBlending = false;
		}
	}
}

void ABattleManager::WarmUpEffects()
{
	// 카메라 뒤쪽 바닥 아래
	FVector WarmupPos = GetActorLocation();
	WarmupPos.X -= 500.f;
	WarmupPos.Z -= 600.f;
	
	TSet<UNiagaraSystem*> SeenFX;
	
	// 플레이어 + 적 파티의 모든 어빌리티 순회
	TArray<ABattleCombatant*> All;
	for (ABattleCombatant* P : PlayerParty) All.Add(P);
	for (ABattleCombatant* E : EnemyParty)  All.Add(E);
	
	for (ABattleCombatant* C : All)
	{
		UAbilitySystemComponent* ASC = C ? C->GetAbilitySystemComponent() : nullptr;
		if (!ASC) continue;

		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			const UTBGameplayAbility* GA = Cast<UTBGameplayAbility>(Spec.Ability.Get());
			if (!GA) continue;

			// HitEffect (Niagara)
			if (GA->HitEffect && !SeenFX.Contains(GA->HitEffect))
			{
				SeenFX.Add(GA->HitEffect);
				UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, GA->HitEffect, WarmupPos);
			}

			// ImpactActorClass → CDO에서 ImpactEffect 읽어 직접 스폰
			if (GA->ImpactActorClass)
			{
				const ABattleImpactActor* CDO = GA->ImpactActorClass->GetDefaultObject<ABattleImpactActor>();
				if (CDO && CDO->ImpactEffect && !SeenFX.Contains(CDO->ImpactEffect))
				{
					SeenFX.Add(CDO->ImpactEffect);
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, CDO->ImpactEffect, WarmupPos);
				}
			}
		}
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

void ABattleManager::SpawnAndStartBattle()
{
	UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance());
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("BattleManager: GameInstance가 없습니다."));
		return;
	}

	// ─── 플레이어 스폰 ───────────────────────────────────────────────────────
	TArray<ABattlePlayerCharacter*> Players;

	if (PlayerSpawnPoints.Num() < GI->PartyData.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("BattleManager: PlayerSpawnPoints(%d)가 PartyData(%d)보다 적습니다."),
			PlayerSpawnPoints.Num(), GI->PartyData.Num());
	}

	for (int32 i = 0; i < GI->PartyData.Num() && i < PlayerSpawnPoints.Num(); i++)
	{
		const FPartyMemberData& Data = GI->PartyData[i];
		AActor* SpawnPoint = PlayerSpawnPoints[i];

		if (!Data.CharacterClass || !SpawnPoint)
		{
			UE_LOG(LogTemp, Warning, TEXT("BattleManager: PartyData[%d] CharacterClass 또는 SpawnPoint가 없습니다."), i);
			continue;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ABattlePlayerCharacter* Player = GetWorld()->SpawnActor<ABattlePlayerCharacter>(
			Data.CharacterClass,
			SpawnPoint->GetActorLocation(),
			SpawnPoint->GetActorRotation(),
			SpawnParams
		);

		if (Player)
		{
			Player->CharacterId = Data.CharacterId;
			Players.Add(Player);
			UE_LOG(LogTemp, Display, TEXT("BattleManager: 플레이어 스폰 - %s (Lv.%d)"),
				*Data.CharacterId.ToString(), Data.Level);
		}
	}

	// ─── 적 스폰 ─────────────────────────────────────────────────────────────
	TArray<ABattleEnemyCharacter*> Enemies;
	const TArray<TSoftClassPtr<ABattleEnemyCharacter>>& EnemyClasses = GI->BattleTransitionData.EnemyClasses;

	if (!EnemySpawnCenter)
	{
		UE_LOG(LogTemp, Error, TEXT("BattleManager: EnemySpawnCenter가 설정되지 않았습니다."));
	}
	else if (EnemyClasses.Num() > 0)
	{
		const int32 EnemyCount = EnemyClasses.Num();
		const FVector CenterLoc = EnemySpawnCenter->GetActorLocation();

		// 중심 기준 좌우 대칭 배치 (Y축)
		// 예: 3마리 → -1, 0, +1 → 간격 곱해서 -200, 0, +200
		const float StartOffset = -((EnemyCount - 1) / 2.f) * EnemySpacing;

		for (int32 i = 0; i < EnemyCount; i++)
		{
			TSubclassOf<ABattleEnemyCharacter> EnemyClass = EnemyClasses[i].LoadSynchronous();
			if (!EnemyClass)
			{
				UE_LOG(LogTemp, Warning, TEXT("BattleManager: EnemyClasses[%d] 로드 실패"), i);
				continue;
			}

			FVector SpawnLoc = CenterLoc;
			SpawnLoc.Y += StartOffset + (i * EnemySpacing);

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			ABattleEnemyCharacter* Enemy = GetWorld()->SpawnActor<ABattleEnemyCharacter>(
				EnemyClass,
				SpawnLoc,
				EnemySpawnRotation,
				SpawnParams
			);

			if (Enemy)
			{
				Enemies.Add(Enemy);
				UE_LOG(LogTemp, Display, TEXT("BattleManager: 적 스폰 - %s"), *Enemy->GetName());
			}
		}
	}

	// ─── 전투 시작 ───────────────────────────────────────────────────────────
	if (Players.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("BattleManager: 스폰된 플레이어가 없습니다."));
		return;
	}
	if (Enemies.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("BattleManager: 스폰된 적이 없습니다."));
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("BattleManager: SpawnAndStartBattle → Player %d명, Enemy %d명"),
		Players.Num(), Enemies.Num());
	StartBattle(Players, Enemies);
}

// ─── 전투 시작 ─────────────────────────────────────────────────────────────────
void ABattleManager::StartBattle(
	const TArray<ABattlePlayerCharacter*>& Players,
	const TArray<ABattleEnemyCharacter*>&  Enemies)
{
	PlayerParty.Reset();
	EnemyParty.Reset();

	UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance());

	for (ABattlePlayerCharacter* P : Players)
	{
		if (!P || P->IsDead()) continue;
		PlayerParty.Add(P);
		P->OnDeath.AddDynamic(this, &ABattleManager::OnCombatantDied);
		P->OnDamageReceived.AddDynamic(this, &ABattleManager::OnCombatantDamaged);
		P->OnHealReceived.AddDynamic(this, &ABattleManager::OnCombatantHealed);

		// GameInstance에서 레벨 스탯 적용
		if (GI)
		{
			FPartyMemberData* PartyMember = GI->GetPartyMemberData(P->CharacterId);
			if (PartyMember)
			{
				FCharacterLevelStats LevelStats;
				if (GI->GetLevelStats(P->CharacterId, PartyMember->Level, LevelStats))
				{
					P->ApplyLevelStats(LevelStats, *PartyMember);
				}
			}
			else
			{
				// 파티 데이터 없으면 기본 레벨 1로 조회
				FCharacterLevelStats LevelStats;
				FPartyMemberData DefaultData;
				DefaultData.CharacterId = P->CharacterId;
				DefaultData.Level = 1;
				if (GI->GetLevelStats(P->CharacterId, 1, LevelStats))
				{
					P->ApplyLevelStats(LevelStats, DefaultData);
				}
			}
		}
	}
	for (ABattleEnemyCharacter* E : Enemies)
	{
		if (!E || E->IsDead()) continue;
		EnemyParty.Add(E);
		E->OnDeath.AddDynamic(this, &ABattleManager::OnCombatantDied);
		E->OnDamageReceived.AddDynamic(this, &ABattleManager::OnCombatantDamaged);
		E->OnHealReceived.AddDynamic(this, &ABattleManager::OnCombatantHealed);
	}

	// 아티펙트 효과 적용
	ApplyArtifacts();

	// 이펙트 웜업 로딩
	WarmUpEffects();

	// HUD 바인딩 (bAutoStartBattle 모드에서도 동작하도록)
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (ATBBattleHUD* HUD = Cast<ATBBattleHUD>(PC->GetHUD()))
		{
			HUD->SetBattleManager(this);
		}
	}

	SetPhase(EBattlePhase::BattleStart);

	// 스탯 적용 완료 → UI 초기화 트리거
	OnBattleReady.Broadcast();

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

// ─── ATB 시스템: 게이지 충전 및 다음 행동자 찾기 ──────────────────────────────
ABattleCombatant* ABattleManager::ChargeAndFindNextActor()
{
	// 살아있는 모든 캐릭터 수집
	TArray<ABattleCombatant*> AllLiving;
	for (ABattleCombatant* P : PlayerParty) if (IsValid(P) && !P->IsDead()) AllLiving.Add(P);
	for (ABattleCombatant* E : EnemyParty)  if (IsValid(E) && !E->IsDead()) AllLiving.Add(E);

	if (AllLiving.IsEmpty()) return nullptr;

	// 게이지가 100 이상인 캐릭터가 나올 때까지 반복 충전
	while (true)
	{
		// 가장 높은 게이지를 가진 캐릭터 찾기
		ABattleCombatant* HighestGauge = nullptr;
		float MaxGauge = -1.f;

		for (ABattleCombatant* C : AllLiving)
		{
			if (C->ActionGauge > MaxGauge)
			{
				MaxGauge = C->ActionGauge;
				HighestGauge = C;
			}
		}

		// 100 이상이면 행동 가능
		if (HighestGauge && HighestGauge->IsActionReady())
		{
			return HighestGauge;
		}

		// 아직 없으면 모두 충전
		for (ABattleCombatant* C : AllLiving)
		{
			C->ChargeActionGauge();
		}
	}
}

// ATB 시스템: 게이지 시뮬레이션으로 다음 N턴 예측
TArray<ABattleCombatant*> ABattleManager::SimulateUpcomingTurns(int32 Count) const
{
	TArray<ABattleCombatant*> Result;
	if (Count <= 0) return Result;

	// 살아있는 캐릭터와 현재 게이지 복사
	TArray<ABattleCombatant*> AllLiving;
	TMap<ABattleCombatant*, float> SimGauges;

	// 현재 턴을 진행 중인 캐릭터가 있다면 0번에 고정
	if (IsValid(CurrentActor) && !CurrentActor->IsDead())
	{
		Result.Add(CurrentActor);

		// 시뮬레이션 상에서 CurrentActor는 이미 행동을 시작한 것으로 간주하여
		// 다음 턴 계산을 위해 게이지를 미리 차감
		if (SimGauges.Contains(CurrentActor))
		{
			SimGauges[CurrentActor] -= ABattleCombatant::ActionGaugeThreshold;
		}
	}

	for (ABattleCombatant* P : PlayerParty)
	{
		if (IsValid(P) && !P->IsDead())
		{
			AllLiving.Add(P);
			SimGauges.Add(P, P->ActionGauge);
		}
	}
	for (ABattleCombatant* E : EnemyParty)
	{
		if (IsValid(E) && !E->IsDead())
		{
			AllLiving.Add(E);
			SimGauges.Add(E, E->ActionGauge);
		}
	}

	if (AllLiving.IsEmpty()) return Result;

	// N명 예측
	while (Result.Num() < Count)
	{
		// 가장 높은 게이지 찾기
		ABattleCombatant* Next = nullptr;
		float MaxGauge = -1.f;

		for (ABattleCombatant* C : AllLiving)
		{
			const float Gauge = SimGauges[C];
			if (Gauge > MaxGauge)
			{
				MaxGauge = Gauge;
				Next = C;
			}
		}

		// 100 이상이면 결과에 추가, 게이지 소모
		if (Next && MaxGauge >= ABattleCombatant::ActionGaugeThreshold)
		{
			Result.Add(Next);
			SimGauges[Next] -= ABattleCombatant::ActionGaugeThreshold;
		}
		else
		{
			// 아직 없으면 모두 충전
			for (ABattleCombatant* C : AllLiving)
			{
				SimGauges[C] += C->GetSpeed();
			}
		}

		// 무한루프 방지
		static int32 MaxIterations = 1000;
		if (Result.Num() == 0 && MaxIterations-- <= 0) break;
	}

	return Result;
}

// ─── 다음 턴으로 이동 (ATB 시스템) ────────────────────────────────────────────
void ABattleManager::AdvanceTurn()
{
	CheckBattleEnd();
	if (CurrentPhase == EBattlePhase::BattleVictory ||
	    CurrentPhase == EBattlePhase::BattleDefeat) return;

	// ATB: 게이지 충전 후 행동 가능한 캐릭터 찾기
	ABattleCombatant* Next = ChargeAndFindNextActor();
	if (!Next)
	{
		// 모든 캐릭터 사망?
		CheckBattleEnd();
		return;
	}

	// 게이지 소모
	Next->ConsumeActionGauge();
	CurrentActor = Next;
	
	BroadcastTurnOrder();

	// 자기 턴이 돌아오면 방어 상태 해제
	Next->SetDefending(false);

	// ─── 상태이상 틱 처리 ────────────────────────────────────────────────────
	bool bStunned = Next->TickStatusEffects();

	// 상태이상으로 인한 사망 확인
	if (Next->IsDead())
	{
		// 0.75초 이후 다음 턴으로 진행
		FTimerHandle DeathSkipTimer;
		GetWorldTimerManager().SetTimer(DeathSkipTimer, this, &ABattleManager::AdvanceTurn, 0.75f, false);
		return;
	}

	BroadcastTurnOrder();
	OnTurnBegin.Broadcast(Next);
	Next->OnTurnBegin();

	if (bStunned)
	{
		// 스턴: 메뉴 열지 않고 0.75초 표시 후 다음 턴으로
		GetWorldTimerManager().SetTimer(
			StunSkipTimer, this, &ABattleManager::OnActionComplete, 0.75f, false);
		return;
	}

	if (Cast<ABattlePlayerCharacter>(Next))
		StartPlayerTurn();
	else if (Cast<ABattleEnemyCharacter>(Next))
		StartEnemyTurn();
}

// ─── 플레이어 턴 ──────────────────────────────────────────────────────────────
void ABattleManager::StartPlayerTurn()
{
	UE_LOG(LogTemp, Log, TEXT("ABattleManager::StartPlayerTurn Enter"));

	PendingTarget       = nullptr;
	PendingAbilityClass = nullptr;
	SetPhase(EBattlePhase::PlayerTurn);
	UE_LOG(LogTemp, Display, TEXT("BattleManager: [PlayerTurn] %s"), GetCurrentActor() ? *GetCurrentActor()->GetName() : TEXT("None"));
}

// ─── 적 턴 ────────────────────────────────────────────────────────────────────
void ABattleManager::StartEnemyTurn()
{
	PendingDiceMultiplier = 1.0f;
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
	PendingTargetType = EAbilityTargetType::SingleEnemy; // 기본 공격은 항상 단일 적
	UE_LOG(LogTemp, Display, TEXT("BattleManager: SetPhase SelectingTarget, Enemies=%d"), GetLivingEnemies().Num());
	SetPhase(EBattlePhase::SelectingTarget);
}

void ABattleManager::PlayerSelectAbility(TSubclassOf<UTBGameplayAbility> AbilityClass)
{
	if (CurrentPhase != EBattlePhase::PlayerTurn) return;
	PendingAbilityClass = AbilityClass;

	// CDO에서 TargetType 확인
	UTBGameplayAbility* CDO = AbilityClass->GetDefaultObject<UTBGameplayAbility>();
	if (!CDO) return;

	PendingTargetType = CDO->TargetType;

	switch (CDO->TargetType)
	{
	case EAbilityTargetType::Self:
		// 타겟 선택 없이 바로 실행 (자기 자신)
		ExecuteAction(GetCurrentActor(), GetCurrentActor(), AbilityClass);
		break;

	case EAbilityTargetType::SingleEnemy:
	case EAbilityTargetType::AllEnemies:
	case EAbilityTargetType::SingleAlly:
	case EAbilityTargetType::AllAllies:
		// UI에서 타겟 선택/확인
		SetPhase(EBattlePhase::SelectingTarget);
		break;
	}
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
	{
		PendingCaster       = Caster;
		PendingTarget       = Target;
		PendingAbilityClass = AbilityToUse;
		RollDiceAndWait();
	}
}

void ABattleManager::PlayerSelectDefend()
{
	if (CurrentPhase != EBattlePhase::PlayerTurn) return;

	ABattleCombatant* Current = GetCurrentActor();
	if (!Current) return;

	Current->SetDefending(true);
	OnActionComplete();
}

void ABattleManager::PlayerCancel()
{
	if (CurrentPhase == EBattlePhase::SelectingTarget)
	{
		PendingAbilityClass = nullptr;
		PendingTarget       = nullptr;
		SetPhase(EBattlePhase::PlayerTurn);
	}
	else if (CurrentPhase == EBattlePhase::DiceRolling)
	{
		// 주사위 결과 타이머 취소 후 플레이어 턴으로 복귀
		GetWorldTimerManager().ClearTimer(DiceResultTimer);
		PendingDiceMultiplier = 1.0f;
		PendingAbilityClass   = nullptr;
		PendingTarget         = nullptr;
		SetPhase(EBattlePhase::PlayerTurn);
	}
}

// ─── 주사위 ──────────────────────────────────────────────────────────────────────
void ABattleManager::RollDiceAndWait()
{
	// 시전자의 장착 주사위 가져오기
	FDiceData Dice;
	if (ABattlePlayerCharacter* PC = Cast<ABattlePlayerCharacter>(PendingCaster))
		Dice = PC->GetEquippedDice();

	// 면이 없으면 기본값(1.0) 유지하고 바로 실행
	if (Dice.BaseFaces.IsEmpty())
	{
		PendingDiceMultiplier = 1.0f;
		ExecuteAction(PendingCaster, PendingTarget, PendingAbilityClass);
		return;
	}

	// 주사위 데이터 저장 (애니메이션용)
	PendingDiceData = Dice;

	// 랜덤으로 면 뽑기 (최종 결과)
	const int32 RawFace = Dice.BaseFaces[FMath::RandRange(0, Dice.BaseFaces.Num() - 1)];

	// GameInstance에서 FaceBonus 읽기
	int32 FaceBonus = 0, MinFace = -10, MaxFace = 10;
	if (UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance()))
	{
		FaceBonus = GI->DiceModifier.FaceBonus;
		MinFace   = GI->DiceModifier.MinFace;
		MaxFace   = GI->DiceModifier.MaxFace;
	}

	// 최종 결과 저장
	PendingDiceFinalFace = FMath::Clamp(RawFace + FaceBonus, MinFace, MaxFace);
	PendingDiceMultiplier = 1.0f + PendingDiceFinalFace * 0.1f;

	SetPhase(EBattlePhase::DiceRolling);

	// 0.1초 간격 애니메이션 타이머 시작
	GetWorldTimerManager().SetTimer(
		DiceAnimationTimer, this, &ABattleManager::DiceAnimationTick, 0.1f, true);

	// 2초 후 애니메이션 종료 및 결과 표시
	GetWorldTimerManager().SetTimer(
		DiceResultTimer, this, &ABattleManager::ExecuteActionAfterDice, 2.0f, false);
}

void ABattleManager::DiceAnimationTick()
{
	if (PendingDiceData.BaseFaces.IsEmpty()) return;

	// 랜덤 면 뽑기
	const int32 RandomFace = PendingDiceData.BaseFaces[FMath::RandRange(0, PendingDiceData.BaseFaces.Num() - 1)];
	const float RandomMultiplier = 1.0f + RandomFace * 0.1f;

	OnDiceRolled.Broadcast(RandomFace, RandomMultiplier);
}

void ABattleManager::ExecuteActionAfterDice()
{
	// 애니메이션 타이머 정지
	GetWorldTimerManager().ClearTimer(DiceAnimationTimer);

	// 최종 결과 브로드캐스트
	OnDiceRolled.Broadcast(PendingDiceFinalFace, PendingDiceMultiplier);

	// 2초 후 액션 실행
	FTimerHandle ActionDelayTimer;
	GetWorldTimerManager().SetTimer(
		ActionDelayTimer,
		[this]() { ExecuteAction(PendingCaster, PendingTarget, PendingAbilityClass); },
		2.0f,
		false);
}

FDiceData ABattleManager::GetCurrentCasterDice() const
{
	// DicePreview UI에서는 현재 턴 캐릭터의 주사위를 보여줘야 함
	if (ABattlePlayerCharacter* PC = Cast<ABattlePlayerCharacter>(GetCurrentActor()))
		return PC->GetEquippedDice();
	return FDiceData{};
}

FText ABattleManager::GetCurrentCasterDiceFacesText() const
{
	const FDiceData Dice = GetCurrentCasterDice();
	FString Result;
	for (int32 i = 0; i < Dice.BaseFaces.Num(); ++i)
	{
		if (i > 0) Result += TEXT(", ");
		Result += FString::FromInt(Dice.BaseFaces[i]);
	}
	return FText::FromString(Result);
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
	PendingCaster = Caster;
	PendingAbilityClass = AbilityClass;

	UTBGameplayAbility* CDO = AbilityClass->GetDefaultObject<UTBGameplayAbility>();
	if (!CDO) { OnActionComplete(); return; }

	PendingTargetType = CDO->TargetType;

	// ─── 캐릭터 회전 (카메라 전환 전에 먼저 수행) ───
	// 회전 전 원래 회전 저장 (FinishAbility에서 복귀용)
	Caster->PreAbilityRotation = Caster->GetActorRotation();

	FVector LookAtLocation = FVector::ZeroVector;
	bool bShouldRotate = false;

	if (CDO->TargetType == EAbilityTargetType::AllEnemies)
	{
		TArray<ABattleCombatant*> Enemies = GetLivingEnemies();
		for (ABattleCombatant* E : Enemies)
			if (E) LookAtLocation += E->GetActorLocation();
		if (!Enemies.IsEmpty())
		{
			LookAtLocation /= Enemies.Num();
			bShouldRotate = true;
		}
	}
	else if (CDO->TargetType == EAbilityTargetType::AllAllies)
	{
		TArray<ABattleCombatant*> Allies = GetLivingPlayers();
		for (ABattleCombatant* A : Allies)
			if (A) LookAtLocation += A->GetActorLocation();
		if (!Allies.IsEmpty())
		{
			LookAtLocation /= Allies.Num();
			bShouldRotate = true;
		}
	}
	else if (Target)
	{
		LookAtLocation = Target->GetActorLocation();
		bShouldRotate = true;
	}

	if (bShouldRotate)
	{
		FVector Dir = LookAtLocation - Caster->GetActorLocation();
		Dir.Z = 0.f;
		if (!Dir.IsNearlyZero())
		{
			Caster->SetActorRotation(Dir.Rotation());
		}
	}

	// ─── 카메라 전환 (캐릭터 회전 후) ───
	float CameraBlendTime = 0.f;

	// Impact + 컷씬 카메라 사용 시 → 액션 카메라 스킵 (어빌리티에서 컷씬 카메라로 직접 전환)
	const bool bSkipActionCamera = (CDO->AnimationType == EAbilityAnimType::Impact && CDO->bUseCutsceneCamera);

	if (CDO->bUseActionCamera && !bSkipActionCamera)
	{
		SwitchToActionCamera(Caster, CDO);
		CameraBlendTime = CDO->CameraBlendInTime;
	}

	// 카메라 블렌드 완료 후 어빌리티 활성화
	if (CameraBlendTime > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			ActionCameraDelayTimer, this, &ABattleManager::ActivatePendingAbility, CameraBlendTime, false);
	}
	else
	{
		ActivatePendingAbility();
	}
}

void ABattleManager::ActivatePendingAbility()
{
	if (!IsValid(PendingCaster) || !PendingAbilityClass)
	{
		OnActionComplete();
		return;
	}

	UAbilitySystemComponent* ASC = PendingCaster->GetAbilitySystemComponent();
	if (!ASC) { OnActionComplete(); return; }

	// ASC에서 해당 클래스의 Spec을 찾아 활성화
	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(PendingAbilityClass);
	if (Spec)
	{
		if (!ASC->TryActivateAbility(Spec->Handle))
		{
			// 비용 부족 등으로 활성화 실패 → 플레이어 턴으로 복귀
			if (Cast<ABattlePlayerCharacter>(PendingCaster))
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
	
	CurrentActor = nullptr; 
	BroadcastTurnOrder(); // 리스트 갱신

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
	return CurrentActor;
}

TArray<ABattleCombatant*> ABattleManager::GetUpcomingTurns(int32 Count) const
{
	return SimulateUpcomingTurns(Count);
}

TArray<ABattleCombatant*> ABattleManager::GetLivingPlayers() const
{
	TArray<ABattleCombatant*> Result;
	for (ABattleCombatant* P : PlayerParty) if (IsValid(P) && !P->IsDead()) Result.Add(P);
	return Result;
}

TArray<ABattleCombatant*> ABattleManager::GetLivingEnemies() const
{
	TArray<ABattleCombatant*> Result;
	for (ABattleCombatant* E : EnemyParty) if (IsValid(E) && !E->IsDead()) Result.Add(E);
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

	if (NewPhase == EBattlePhase::PlayerTurn)
		SwitchToPlayerTurnCamera();
	else if (NewPhase == EBattlePhase::SelectingTarget)
		ReturnToBattleCamera();

	OnBattlePhaseChanged.Broadcast(NewPhase);
}

void ABattleManager::CheckBattleEnd()
{
	// 이미 종료 페이즈면 중복 실행 방지
	if (CurrentPhase == EBattlePhase::BattleVictory ||
	    CurrentPhase == EBattlePhase::BattleDefeat)
		return;

	if (GetLivingEnemies().IsEmpty())
	{
		SetPhase(EBattlePhase::BattleVictory);
		HandleBattleVictory();
		// VictoryWidget에서 ReturnToWorld() 호출 시 ExitBattleMode() 처리
	}
	else if (GetLivingPlayers().IsEmpty())
	{
		SetPhase(EBattlePhase::BattleDefeat);
		HandleBattleDefeat();
		//게임오버
	}
}

void ABattleManager::BroadcastTurnOrder()
{
	OnTurnOrderUpdated.Broadcast(GetUpcomingTurns(5));
}

// ─── 패링 ─────────────────────────────────────────────────────────────────────
void ABattleManager::OpenParryTiming(float Duration)
{
	bParryTimingOpen = true;
	GetWorldTimerManager().SetTimer(ParryTimingTimer, this, &ABattleManager::CloseParryTiming, Duration, false);
}

bool ABattleManager::TryParry()
{
	// 쿨다운 중이면 입력 무시 (선입력 페널티)
	if (bParryCooldown) return false;

	// 패링 시도 시 무조건 쿨다운 시작 (성공/실패 관계없이)
	bParryCooldown = true;
	GetWorldTimerManager().SetTimer(ParryCooldownTimer, this, &ABattleManager::ClearParryCooldown, ParryCooldownDuration, false);

	// 타이밍이 안 열려있으면 실패 (쿨다운만 적용)
	if (!bParryTimingOpen) return false;

	// PendingTargetType 기준으로 패링 대상 결정
	// AllEnemies(적이 전체 공격) → 살아있는 플레이어 전원, 그 외 → PendingTarget만
	TArray<ABattleCombatant*> ParryTargets;
	if (PendingTargetType == EAbilityTargetType::AllEnemies)
	{
		for (ABattlePlayerCharacter* P : PlayerParty)
			if (P && !P->IsDead()) ParryTargets.Add(P);
	}
	else
	{
		if (PendingTarget && !PendingTarget->IsDead())
			ParryTargets.Add(PendingTarget);
	}

	for (ABattleCombatant* Target : ParryTargets)
	{
		if (UAbilitySystemComponent* ASC = Target->GetAbilitySystemComponent())
			ASC->AddLooseGameplayTag(TAG_Combatant_State_ParrySuccess);
		Target->PlayParryMontage();
	}

	CloseParryTiming();
	return !ParryTargets.IsEmpty();
}

void ABattleManager::CloseParryTiming()
{
	bParryTimingOpen = false;
	GetWorldTimerManager().ClearTimer(ParryTimingTimer);
}

void ABattleManager::ClearParryCooldown()
{
	bParryCooldown = false;
	GetWorldTimerManager().ClearTimer(ParryCooldownTimer);
}

void ABattleManager::OnCombatantDied(ABattleCombatant* Combatant)
{
	BroadcastTurnOrder();
	CheckBattleEnd();
}

void ABattleManager::OnCombatantDamaged(ABattleCombatant* Combatant, float Damage, bool /*bIsCritical*/)
{
	OnAnyDamageDealt.Broadcast(Combatant, Damage);
}

void ABattleManager::OnCombatantHealed(ABattleCombatant* Combatant, float Heal)
{
	OnAnyHealDealt.Broadcast(Combatant, Heal);
}

// ─── 카메라 전환 ──────────────────────────────────────────────────────────────
void ABattleManager::SwitchToPlayerTurnCamera()
{
	if (!ActionCamera) return;

	ABattleCombatant* Current = GetCurrentActor();
	if (!Current) return;

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	const FTransform CasterTransform = Current->GetActorTransform();
	const FVector    WorldLocation   = CasterTransform.TransformPosition(PlayerTurnCameraOffset);
	const FRotator   WorldRotation   = CasterTransform.TransformRotation(
		PlayerTurnCameraRotation.Quaternion()).Rotator();

	ActionCamera->SetActorLocation(WorldLocation);
	ActionCamera->SetActorRotation(WorldRotation);
	ActionCamera->AttachToActor(Current, FAttachmentTransformRules::KeepWorldTransform);

	PendingCameraBlendOutTime = PlayerTurnCameraBlendTime;
	bActionCameraActive = true;

	PC->SetViewTargetWithBlend(ActionCamera, PlayerTurnCameraBlendTime, VTBlend_Cubic);
}

void ABattleManager::SwitchToActionCamera(ABattleCombatant* Caster, const UTBGameplayAbility* AbilityCDO)
{
	if (!ActionCamera || !Caster || !AbilityCDO) return;

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	// 기존 Attach 해제
	ActionCamera->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// 캐릭터 로컬 오프셋 → 월드 좌표로 변환
	const FTransform CasterTransform = Caster->GetActorTransform();
	const FVector    WorldLocation   = CasterTransform.TransformPosition(AbilityCDO->ActionCameraLocalOffset);
	const FRotator   WorldRotation   = CasterTransform.TransformRotation(
		AbilityCDO->ActionCameraLocalRotation.Quaternion()).Rotator();

	ActionCamera->SetActorLocation(WorldLocation);
	ActionCamera->SetActorRotation(WorldRotation);

	// Attach 제거 — 카메라는 월드 좌표로 고정, 캐릭터 회전에 영향 안 받음

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

void ABattleManager::SetActionCameraWorldPosition(const FVector& WorldLocation, const FRotator& WorldRotation, float BlendTime)
{
	if (!ActionCamera) return;

	// 기존 Attach 해제 (자유 이동 가능하게)
	ActionCamera->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	// 아직 ActionCamera가 ViewTarget이 아니면 전환
	if (!bActionCameraActive)
	{
		// 블렌딩 전에 현재 카메라 위치에서 시작하도록 설정
		if (AActor* CurrentViewTarget = PC->GetViewTarget())
		{
			ActionCamera->SetActorLocation(CurrentViewTarget->GetActorLocation());
			ActionCamera->SetActorRotation(CurrentViewTarget->GetActorRotation());
		}

		bActionCameraActive = true;
		PC->SetViewTargetWithBlend(ActionCamera, BlendTime, VTBlend_Cubic);
	}

	// 시작 위치 저장 (현재 ActionCamera 위치)
	CutsceneCameraStartLocation = ActionCamera->GetActorLocation();
	CutsceneCameraStartRotation = ActionCamera->GetActorRotation();

	// 목표 위치로 부드럽게 이동 (Tick에서 보간)
	CutsceneCameraTargetLocation = WorldLocation;
	CutsceneCameraTargetRotation = WorldRotation;
	CutsceneCameraBlendTime = FMath::Max(0.01f, BlendTime);
	CutsceneCameraBlendElapsed = 0.f;
	bCutsceneCameraBlending = true;
}

// ─── 전투 종료 처리 ────────────────────────────────────────────────────────────

void ABattleManager::HandleBattleVictory()
{
	UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance());
	if (!GI) return;

	// ─── 경험치 분배 전 데이터 저장 ─────────────────────────────────────────────
	TArray<FCharacterExpData> BeforeExpData;
	TMap<FName, int32> OldLevels;
	TMap<FName, int32> OldExps;

	for (const FPartyMemberData& Member : GI->PartyData)
	{
		OldLevels.Add(Member.CharacterId, Member.Level);
		OldExps.Add(Member.CharacterId, Member.CurrentExp);

		FCharacterExpData ExpData;
		ExpData.CharacterId = Member.CharacterId;
		ExpData.Level = Member.Level;
		ExpData.CurrentExp = Member.CurrentExp;
		ExpData.ExpToNextLevel = FPartyMemberData::GetRequiredExpForLevel(Member.Level);
		ExpData.bLeveledUp = false;
		BeforeExpData.Add(ExpData);
	}

	// ─── 총 경험치 계산 (죽은 적 전체) ─────────────────────────────────────────
	int32 TotalExp = 0;
	for (ABattleEnemyCharacter* Enemy : EnemyParty)
	{
		if (Enemy)
			TotalExp += Enemy->ExpReward;
	}

	// ─── 파티에 경험치 분배 ────────────────────────────────────────────────────
	GI->AddExpToParty(TotalExp);

	// ─── 경험치 분배 후 데이터 수집 ────────────────────────────────────────────
	TArray<FCharacterExpData> AfterExpData;
	TArray<FLevelUpDisplayData> LevelUpData;

	for (const FPartyMemberData& Member : GI->PartyData)
	{
		int32* OldLevel = OldLevels.Find(Member.CharacterId);
		bool bLeveledUp = OldLevel && Member.Level > *OldLevel;

		// AfterExpData 구성
		FCharacterExpData ExpData;
		ExpData.CharacterId = Member.CharacterId;
		ExpData.Level = Member.Level;
		ExpData.CurrentExp = Member.CurrentExp;
		ExpData.ExpToNextLevel = FPartyMemberData::GetRequiredExpForLevel(Member.Level);
		ExpData.GainedExp = TotalExp / GI->PartyData.Num();  // 균등 분배된 경험치
		ExpData.bLeveledUp = bLeveledUp;
		AfterExpData.Add(ExpData);

		// 레벨업 UI 데이터		
		FLevelUpDisplayData Data;
		Data.CharacterId = Member.CharacterId;
		Data.OldLevel = *OldLevel;
		Data.NewLevel = Member.Level;
		Data.bLeveledUp = bLeveledUp;
		GI->GetLevelStats(Member.CharacterId, *OldLevel, Data.OldStats);
		GI->GetLevelStats(Member.CharacterId, Member.Level, Data.NewStats);
		LevelUpData.Add(Data);		
	}

	// 파티 스탯 저장
	SavePartyStats();

	// 데이터 캐싱 후 3초 딜레이로 위젯 표시
	CachedBeforeExpData = BeforeExpData;
	CachedAfterExpData = AfterExpData;
	CachedLevelUpData = LevelUpData;

	GetWorldTimerManager().SetTimer(
		VictoryWidgetTimer,
		this,
		&ABattleManager::ShowVictoryWidgetDelayed,
		3.0f,
		false
	);
}

void ABattleManager::ShowVictoryWidgetDelayed()
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (ATBBattleHUD* HUD = Cast<ATBBattleHUD>(PC->GetHUD()))
		{
			HUD->ShowVictoryWidget(CachedBeforeExpData, CachedAfterExpData, CachedLevelUpData);
		}
	}
}

void ABattleManager::HandleBattleDefeat()
{
	// 패배 시에도 현재 스탯은 저장 (부활 처리는 필드 복귀 시)
	SavePartyStats();
}

void ABattleManager::SavePartyStats()
{
	UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance());
	if (!GI) return;

	for (ABattlePlayerCharacter* Player : PlayerParty)
	{
		if (!Player) continue;

		// 사망한 캐릭터는 HP 0으로 저장 (필드 복귀 시 HP 1로 부활)
		const float HP = Player->IsDead() ? 0.f : Player->GetHP();
		const float MP = Player->GetMP();
		const float Stamina = Player->GetStamina();

		GI->SavePartyMemberStats(Player->CharacterId, HP, MP, Stamina);
	}
}

void ABattleManager::ApplyArtifacts()
{
	UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance());
	if (!GI) return;

	// 아티팩트 정보 가져오기
	for (const FName& ArtifactID : GI->PartyArtifactData.EquippedArtifact)
	{
		FArtifactData ArtifactRow;
		if (!GI->GetArtifactRow(ArtifactID, ArtifactRow))
		{
			continue;
		}

		// 적용할 대상 설정
		TArray<ABattlePlayerCharacter*> Targets = GetArtifactTargets(ArtifactRow);

		for (ABattlePlayerCharacter* Target : Targets)
		{
			// 적용
			ApplyArtifactRowToCombatant(ArtifactID, ArtifactRow, Target);
		}
	}
	
}

TArray<ABattlePlayerCharacter*> ABattleManager::GetArtifactTargets(const FArtifactData& ArtifactRow) const
{
	TArray<ABattlePlayerCharacter*> Result;

	switch (ArtifactRow.TargetMode)
	{
	case EArtifactTargetMode::AllParty:
		for (ABattlePlayerCharacter* Player : PlayerParty)
		{
			if (IsValid(Player) && !Player->IsDead())
			{
				Result.Add(Player);
			}
		}
		break;

	case EArtifactTargetMode::MatchingJob:
		for (ABattlePlayerCharacter* Player : PlayerParty)
		{
			if (!IsValid(Player) || Player->IsDead())
			{
				continue;
			}

			if (ArtifactRow.TargetJobIds.Contains(Player->CharacterId))
			{
				Result.Add(Player);
			}
		}
		break;
	}

	return Result;
}

void ABattleManager::ApplyArtifactRowToCombatant(FName ArtifactID, const FArtifactData& ArtifactRow, ABattlePlayerCharacter* Target)
{
	if (Target == nullptr) return;

	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	if (TargetASC == nullptr) return;

	// GE 생성
	UGameplayEffect* GEObj = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None);
	GEObj->DurationPolicy = EGameplayEffectDurationType::Infinite;

	// GE Modifier 설정
	for (const FArtifactStatModifier& Modifier : ArtifactRow.StatModifiers)
	{
		FGameplayModifierInfo ModInfo;
		ModInfo.Attribute = Modifier.Attribute;
		ModInfo.ModifierOp = Modifier.ModifierOp;
		ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(Modifier.Value);
		GEObj->Modifiers.Add(ModInfo);
	}

	// GE Spec 생성, 적용
	FGameplayEffectSpec Spec(GEObj, TargetASC->MakeEffectContext(), 1.f);
	FActiveGameplayEffectHandle Handle = TargetASC->ApplyGameplayEffectSpecToSelf(Spec);

	// GE 핸들 관리. 하나의 아티팩트에도 여러 캐릭터가 적용될 수 있으므로 FindOrAdd
	EquippedArtifactEffect.FindOrAdd(Target->CharacterId).Add(Handle);
}
