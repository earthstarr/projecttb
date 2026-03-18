// Fill out your copyright notice in the Description page of Project Settings.


#include "World/LevelActor/Portal/BossPreparationPortal.h"

#include "TBGameInstance.h"

void ABossPreparationPortal::PortalActivate()
{
	if (UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance()))
	{
		// BossPreparation 입구 포탈은 층 증가로 치지 않도록 1 보정
		GI->DecreasePortalMoveCount();
	}
	
	Super::PortalActivate();
}
