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
	// 전투 맵 세팅 먼저
	UPortalManager* PortalManager = GetWorld()->GetSubsystem<UPortalManager>();
	if (PortalManager != nullptr)
	{
		PortalManager->SetBattleTransitionData(BattleTransitionData);
	}
	
	// 로딩
	Super::PortalActivate();
}

void ABattlePortal::CleanRoom()
{
	Super::CleanRoom();
}
