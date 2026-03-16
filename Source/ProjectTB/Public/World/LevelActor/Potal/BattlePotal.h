// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TBGameInstance.h"
#include "World/DataStruct/RoomDataStruct.h"
#include "World/LevelActor/Potal/PotalBase.h"
#include "BattlePotal.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTTB_API ABattlePotal : public APotalBase
{
	GENERATED_BODY()
	
public:
	ABattlePotal();
	virtual void BeginPlay();
	
	EBattleType BattleType;
	
protected:
	virtual void PotalActivate() override;
	void CleanRoom() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Battle")
	FEnemyGroupData EnemyGroupData;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Battle")
	FBattleTransitionData BattleTransitionData;
};
