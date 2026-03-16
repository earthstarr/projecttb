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
	// 포탈 매니저에 저장해둔 이벤트 방 목록 중에서 가져오기
	UFUNCTION(BlueprintCallable)
	void SetEventType();
	
protected:
	virtual void BeginPlay() override;
	virtual void PotalActivate() override;
	void CleanRoom() override;
};
