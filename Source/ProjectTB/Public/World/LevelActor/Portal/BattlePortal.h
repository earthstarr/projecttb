// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TBGameInstance.h"
#include "World/DataStruct/RoomDataStruct.h"
#include "World/LevelActor/Portal/PortalBase.h"
#include "BattlePortal.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTTB_API ABattlePortal : public APortalBase
{
	GENERATED_BODY()
	
public:
	ABattlePortal();
	virtual void BeginPlay();
	
	EBattleType BattleType;
	
protected:
	virtual void PortalActivate() override;
	void CleanRoom() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Battle")
	FEnemyGroupData EnemyGroupData;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Battle")
	FBattleTransitionData BattleTransitionData;
};
