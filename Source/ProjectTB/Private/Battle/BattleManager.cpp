#include "Battle/BattleManager.h"
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

	WarmUpEffects();
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

// ─── ATB 시스템: 게이지 충전 및 다음 행동자 찾기 ──────────────────────────────
ABattleCombatant* ABattleManager::ChargeAndFindNextActor()
{
	// 살아있는 모든 캐릭터 수집
	TArray<ABattleCombatant*> AllLiving;
	for (ABattleCombatant* P : PlayerParty) if (P && !P->IsDead()) AllLiving.Add(P);
	for (ABattleCombatant* E : EnemyParty)  if (E && !E->IsDead()) AllLiving.Add(E);

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
	if (CurrentActor && !CurrentActor->IsDead())
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
		if (P && !P->IsDead())
		{
			AllLiving.Add(P);
			SimGauges.Add(P, P->ActionGauge);
		}
	}
	for (ABattleCombatant* E : EnemyParty)
	{
		if (E && !E->IsDead())
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
		ExecuteAction(Caster, Target, AbilityToUse);
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

	if (NewPhase == EBattlePhase::PlayerTurn)
		SwitchToPlayerTurnCamera();
	else if (NewPhase == EBattlePhase::SelectingTarget)
		ReturnToBattleCamera();

	OnBattlePhaseChanged.Broadcast(NewPhase);
}

void ABattleManager::CheckBattleEnd()
{
	if (GetLivingEnemies().IsEmpty())
	{
		SetPhase(EBattlePhase::BattleVictory);
		HandleBattleVictory();
	}
	else if (GetLivingPlayers().IsEmpty())
	{
		SetPhase(EBattlePhase::BattleDefeat);
		HandleBattleDefeat();
	}
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

void ABattleManager::SetActionCameraWorldPosition(const FVector& WorldLocation, const FRotator& WorldRotation, float BlendTime)
{
	if (!ActionCamera) return;

	// 기존 Attach 해제 (자유 이동 가능하게)
	ActionCamera->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// 새 위치로 이동
	ActionCamera->SetActorLocation(WorldLocation);
	ActionCamera->SetActorRotation(WorldRotation);

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	// 아직 ActionCamera가 ViewTarget이 아니면 전환
	if (!bActionCameraActive)
	{
		bActionCameraActive = true;
		PC->SetViewTargetWithBlend(ActionCamera, BlendTime, VTBlend_Cubic);
	}
	// 이미 활성화 상태면 위치만 변경됨 (부드럽게 보간됨)
}

// ─── 전투 종료 처리 ────────────────────────────────────────────────────────────

void ABattleManager::HandleBattleVictory()
{
	UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance());
	if (!GI) return;

	// 1. 총 경험치 계산 (죽은 적 전체)
	int32 TotalExp = 0;
	for (ABattleEnemyCharacter* Enemy : EnemyParty)
	{
		if (Enemy)
			TotalExp += Enemy->ExpReward;
	}

	// 2. 파티에 경험치 분배
	GI->AddExpToParty(TotalExp);

	// 3. 파티 스탯 저장
	SavePartyStats();
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
