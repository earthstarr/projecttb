// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "World/LevelActor/Portal/PortalBase.h"
#include "BossPreparationPortal.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTTB_API ABossPreparationPortal : public APortalBase
{
	GENERATED_BODY()
	
protected:
	virtual void PortalActivate() override;
	
};
