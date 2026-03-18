// Fill out your copyright notice in the Description page of Project Settings.


#include "World/LevelActor/Portal/BattlePortal.h"

#include "Kismet/GameplayStatics.h"
#include "World/PortalManager.h"


ABattlePortal::ABattlePortal() : BattleType(EBattleType::Normal)
{
	
}

void ABattlePortal::BeginPlay()
{
	Super::BeginPlay();
	
	if (EnemyGroupData.EnemyClasses.Num() == 0)
	{
		UPortalManager* PortalManager = GetWorld()->GetSubsystem<UPortalManager>();
		if (PortalManager != nullptr)
		{
			// 적 배치 정보 가져오기
			PortalManager->GetRandomEnemyGroup(BattleType , EnemyGroupData);
			BattleTransitionData.EnemyClasses = EnemyGroupData.EnemyClasses;
		}
	}
}

void ABattlePortal::PortalActivate()
{
	UPortalManager* PortalManager = GetWorld()->GetSubsystem<UPortalManager>();
	UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance());

	// 기본값을 복사해서 이번 진입에서만 덮어쓴다.
	FBattleTransitionData TransitionData = BattleTransitionData;

	if (PortalManager != nullptr && GI != nullptr)
	{
		// Super::PortalActivate()에서 PortalMoveCount가 1 증가하므로 미리 다음 값을 계산
		const int32 NextPortalMoveCount = GI->GetPortalMoveCount() + 1;

		// 13층 -> 14층 전투 진입이면, 전투 종료 후 보스 대기룸으로 복귀하게 만든다.
		if (NextPortalMoveCount == 14)
		{
			const FDataTableRowHandle BossPreparationHandle = PortalManager->GetBossPreparationRoomHandle();
			if (!BossPreparationHandle.IsNull())
			{
				TransitionData.PostBattleRoomData = BossPreparationHandle;
			}
		}

		PortalManager->SetBattleTransitionData(TransitionData);
	}
	// 로딩
	Super::PortalActivate();
}

void ABattlePortal::CleanRoom()
{
	Super::CleanRoom();
}
