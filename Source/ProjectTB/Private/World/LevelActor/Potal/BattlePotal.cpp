// Fill out your copyright notice in the Description page of Project Settings.


#include "World/LevelActor/Potal/BattlePotal.h"

#include "Kismet/GameplayStatics.h"
#include "World/PotalManager.h"


ABattlePotal::ABattlePotal()
{
	
}

void ABattlePotal::PotalActivate()
{
	// 전투 맵 세팅 먼저
	UPotalManager* PotalManager = GetWorld()->GetSubsystem<UPotalManager>();
	PotalManager->SetBattleTransitionData(BattleTransitionData);
	
	// 로딩
	Super::PotalActivate();
}

void ABattlePotal::CleanRoom()
{
	Super::CleanRoom();
}
