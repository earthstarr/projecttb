// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "World/LevelActor/Potal/PotalBase.h"
#include "EventPotal.generated.h"

/**
 * 
 */



UCLASS()
class PROJECTTB_API AEventPotal : public APotalBase
{
	GENERATED_BODY()

public:
	
protected:
	virtual void BeginPlay() override;
	virtual void PotalActivate() override;
	void CleanRoom() override;
};
