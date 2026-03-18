// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DataStruct/RoomDataStruct.h"
#include "Subsystems/WorldSubsystem.h"
#include "PortalManager.generated.h"


class APortalSpawnAnchorSet;
class ATargetPoint;
class UPortalSpawnConfig;
class AWorldPlayerController;
class ULevelStreamingDynamic;

struct FBattleTransitionData;
struct FRoomData;
struct FEnemyGroupData;

/**
 * 
 */


UENUM(BlueprintType)
enum class EEventRoomType : uint8
{
	Battle,
	Event,
	Shop
};

UCLASS()
class PROJECTTB_API UPortalManager : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
	UFUNCTION()
	void InitRoomLoad();
	
	UFUNCTION()
	void OnLevelLoadStarted(const FDataTableRowHandle& SelectedRoomHandle);
	
	UFUNCTION()
	void OnReturnToWorldLevel(const FDataTableRowHandle& PostBattleRoomData);
	
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
	
	UFUNCTION()
	TArray<FDataTableRowHandle> GetCachedEventRoomCandidates() { return EventRoomCandidates; }
	
	// 전투 방 종류에 따라 랜덤 적 조합을 제공
	UFUNCTION()
	bool GetRandomEnemyGroup(EBattleType BattleType, FEnemyGroupData& OutData);

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
	
	// 적 정보 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Battle Data", meta=(AllowPrivateAccess="true"))
	TSoftObjectPtr<UDataTable> EnemyDataTable;
	
#pragma region MakePortal
	// 새로운 포탈 생성
	UPROPERTY()
	UPortalSpawnConfig* CachePortalConfig;
	
	void MakeNewPortal();
	
	UPROPERTY()
	TArray<FDataTableRowHandle> BattleRoomCandidates;

	UPROPERTY()
	TArray<FDataTableRowHandle> EventRoomCandidates;

	UPROPERTY()
	TArray<FDataTableRowHandle> ShopRoomCandidates;

	// 방 종류별로 후보 방 정보 캐시화
	void CachePortalRoomCandidates(UDataTable* RoomTable);
	
	// 방 종류에 따라 랜덤한 후보 방 가져오기
	bool GetRandomRoomHandleByPortalType(EEventRoomType PortalType, FDataTableRowHandle& OutHandle) const;
	
	// 앵커에서 포탈이 생성될 수 있는 위치 가져오기
	void CollectPortalSpawnPoints(ULevel* LoadedLevel, TArray<ATargetPoint*>& OutSpawnPoints) const;
	
	// 포탈 생성 및 랜덤 종류 부여
	bool SpawnPortalAtPoint(ATargetPoint* SpawnPoint, ULevel* LoadedLevel, EEventRoomType PortalType);
	
	// 필터링 된 후보 중 하나 선택
	bool GetRoomHandleBasedOnPortalMovement(const TArray<FDataTableRowHandle>* Candidates, FDataTableRowHandle& OutHandle) const;
	
	// 필터링 된 방이 실제로 생성 가능한지 확인
	bool CanSpawnPortalType(EEventRoomType PortalType) const;
	
	// 현재 층 수 기준으로 후보 방 필터링
	void FilterCandidatesByPortalMoveCount(const TArray<FDataTableRowHandle>* Candidates, TArray<FDataTableRowHandle>& OutFilteredCandidates) const;
	
	// 포탈이 생성될 수 있는 위치 관련
public:
	UFUNCTION()
	void RegisterPortalSpawnAnchorSet(APortalSpawnAnchorSet* AnchorSet);

	UFUNCTION()
	void UnregisterPortalSpawnAnchorSet(APortalSpawnAnchorSet* AnchorSet);
	
private:
	UPROPERTY()
	TArray<TWeakObjectPtr<APortalSpawnAnchorSet>> RegisteredPortalSpawnAnchorSets;
	
	bool bPendingPortalGeneration = false;
	bool bPortalGeneratedForCurrentRoom = false;
	
	UPROPERTY()
	TWeakObjectPtr<APortalSpawnAnchorSet> CurrentPortalSpawnAnchorSet;
	
#pragma endregion
};
