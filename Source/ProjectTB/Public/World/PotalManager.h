// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataStruct/RoomDataStruct.h"
#include "Subsystems/WorldSubsystem.h"
#include "PotalManager.generated.h"

struct FBattleTransitionData;
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
	void OnFadeInFinished();
	
	UFUNCTION()
	void OnLevelShown();
	
	UFUNCTION()
	void TeleportLevel();
	
	//배틀 맵 정보 제공
	UFUNCTION()
	void SetBattleTransitionData(FBattleTransitionData& InBattleTransitionData);
	
	// 배틀 매니저 탐색
#pragma region BattleManager
	void StartBattleManagerSearch();
	void StopBattleManagerSearch();
	
	UFUNCTION()
	void TryBattleManagerSearch();
	
	FTimerHandle BattleManagerSearchTimerHandle;
	float BattleManagerSearchElapsed = 0.f;
	
	static constexpr float BattleManagerSearchInterval = 0.2f;
	static constexpr float BattleManagerSearchTimeout = 5.0f;
#pragma endregion
	
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
	
	FTimerHandle FadeInTimerHandle;
	
	// 레벨 언로드, 로드
	FRoomData* RoomData;
	
	UPROPERTY()
	ULevelStreamingDynamic* CurrentLevelInstance;
	
	UPROPERTY()
	ULevelStreamingDynamic* NextLevelInstance;
	
	FDataTableRowHandle PendingRoomHandle;
};
