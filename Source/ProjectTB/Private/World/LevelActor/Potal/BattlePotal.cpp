// Fill out your copyright notice in the Description page of Project Settings.


#include "World/LevelActor/Potal/BattlePotal.h"

#include "Kismet/GameplayStatics.h"
#include "World/PotalManager.h"


ABattlePotal::ABattlePotal()
{
	
}

void ABattlePotal::BeginPlay()
{
	Super::BeginPlay();
	
	if (EnemyGroupData.EnemyClasses.Num() == 0)
	{
		UPotalManager* PotalManager = GetWorld()->GetSubsystem<UPotalManager>();
		if (PotalManager != nullptr)
		{
			// 적 배치 정보 가져오기
			PotalManager->GetRandomEnemyGroup(BattleType , EnemyGroupData);
			BattleTransitionData.EnemyClasses = EnemyGroupData.EnemyClasses;
		}
	}
}

void ABattlePotal::PotalActivate()
{
	// 전투 맵 세팅 먼저
	UPotalManager* PotalManager = GetWorld()->GetSubsystem<UPotalManager>();
	if (PotalManager != nullptr)
	{
		PotalManager->SetBattleTransitionData(BattleTransitionData);
	}
	
	// 로딩
	Super::PotalActivate();
}

void ABattlePotal::CleanRoom()
{
	Super::CleanRoom();
}
