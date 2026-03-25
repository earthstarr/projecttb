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
class APortalBase;

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

	UPROPERTY()
	ULevelStreamingDynamic* PendingUnloadLevelInstance;
	
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
	bool bPendingPlayerTeleport = false;
	bool bPendingRoomActivation = false;
	
	UPROPERTY()
	TWeakObjectPtr<APortalSpawnAnchorSet> CurrentPortalSpawnAnchorSet;

	bool ShouldWaitForAnchorPlayerSpawn() const;
	bool TryGetAnchorPlayerSpawnTransform(FVector& OutLocation, FRotator& OutRotation) const;
	void CommitLevelInstanceChange();
	void UnloadPendingLevelInstance();
	void TryTeleportDeferredPlayer();
	void TryActivateRoomMode();
	
#pragma endregion
	
#pragma region BossRoom
public:
	// 배틀 포탈에서 14층 진입 여부를 판단할 때 보스 대기방 handle을 꺼내 쓴다.
	UFUNCTION()
	FDataTableRowHandle GetBossPreparationRoomHandle() const { return BossPreparationRoomHandle; }
	
private:
	// 보스 대기룸
	FDataTableRowHandle BossPreparationRoomHandle;
	
	// 현재 방 상태에 맞춰 실제 생성할 포탈 종류를 결정
	// 앵커가 늦게 등록됐을 때도 이 함수를 다시 호출하면 같은 규칙으로 재시도
	void TryGeneratePortalForCurrentRoom();
	
	// 고정 목적지 포탈을 생성할 때, 포탈 종류 enum 대신 실제 사용할 포탈 클래스를 직접 받기
	bool SpawnFixedPortalAtPoint(ATargetPoint* SpawnPoint, ULevel* LoadedLevel, const TSoftClassPtr<APortalBase>& PortalSoftClass, const FDataTableRowHandle& FixedHandle);

	// 14층 비전투 방에서 보스 대기룸으로 가는 포탈 하나 생성
	void MakeBossPreparationEntrancePortal();

	// 현재 이동 횟수 조회 헬퍼
	int32 GetCurrentPortalMoveCount() const;
	
#pragma endregion
};
