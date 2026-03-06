// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataStruct/RoomDataStruct.h"
#include "Subsystems/WorldSubsystem.h"
#include "PotalManager.generated.h"

class AWorldPlayerController;
struct FRoomData;
class ULevelStreamingDynamic;
/**
 * 
 */
UCLASS()
class PROJECTTB_API UPotalManager : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
	UFUNCTION()
	void InitRoomLoad();
	
	UFUNCTION()
	void OnLevelLoadStarted(const FDataTableRowHandle& SelectedRoomHandle);
	
	UFUNCTION()
	void OnReturnToWorldLevel(const FDataTableRowHandle& ReturnRoomData);
	
	UFUNCTION()
	void OnLevelLoadFinished();
	
	UFUNCTION()
	void TeleportLevel();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Room Data")
	FDataTableRowHandle InitRoomHandle;
	
	UFUNCTION()
	void ActivateHUDMode(const ERoomType RoomType);
	
private:
	// 페이드 인, 아웃
	UPROPERTY()
	APlayerCameraManager* CamManager;
	
	UPROPERTY()
	AWorldPlayerController* PC;
	
	float TransitionStartTime;
	
	// 레벨 언로드, 로드
	FRoomData* RoomData;
	
	UPROPERTY()
	ULevelStreamingDynamic* CurrentLevelInstance;
	
	UPROPERTY()
	ULevelStreamingDynamic* NextLevelInstance;
};
