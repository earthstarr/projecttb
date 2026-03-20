// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalSpawnAnchorSet.generated.h"

class ATargetPoint;

UCLASS()
class PROJECTTB_API APortalSpawnAnchorSet : public AActor
{
	GENERATED_BODY()
	
public:
	UFUNCTION()
	void GetValidPortalSpawnPoints(TArray<ATargetPoint*>& OutPoints) const;

	UFUNCTION()
	ATargetPoint* GetPlayerSpawnPoint() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//실제 배치될 포탈 위치. 에디터에서 설정해야함
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Portal")
	TArray<TObjectPtr<ATargetPoint>> PortalSpawnPoints;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Portal")
	TObjectPtr<ATargetPoint> PlayerSpawnPoint;
};
