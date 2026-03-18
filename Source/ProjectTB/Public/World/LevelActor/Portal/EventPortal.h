// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "World/LevelActor/Portal/PortalBase.h"
#include "EventPortal.generated.h"

/**
 * 
 */



UCLASS()
class PROJECTTB_API AEventPortal : public APortalBase
{
	GENERATED_BODY()

public:
	
protected:
	virtual void BeginPlay() override;
	virtual void PortalActivate() override;
	void CleanRoom() override;
};
