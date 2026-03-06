// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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

protected:
	virtual void PotalActivate() override;
	void CleanRoom() override;
};
