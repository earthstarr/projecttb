#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RoomDataStruct.generated.h"

class ABattleEnemyCharacter;

UENUM(BlueprintType)
enum class EBattleType : uint8
{
	Normal,
	Elite,
	Boss,
};

UENUM(BlueprintType)
enum class ERoomType : uint8
{
	Test,
	Battle,
	World,
	Event,
	Shop,
	Boss
};

USTRUCT(BlueprintType)
struct FRoomData : public FTableRowBase
{
	GENERATED_BODY()
	
public:
	// 방 종류 (전투, 상점, 이벤트)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData")
	ERoomType RoomType;
	
	// 이동할 레벨
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData")
	TSoftObjectPtr<UWorld> NextLevel;
	
	// 스폰 위치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData")
	FVector StartPosition;
	
	// 스폰 회전 값
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData")
	FRotator StartRotation;
	
	// 최소 포탈 이동 횟수. 이 값 이상일 때만 후보에 포함
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData")
	int32 MinPortalMoveCount = 0;

	// 최대 포탈 이동 횟수. -1이면 상한 없음
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData")
	int32 MaxPortalMoveCount = -1;
};

USTRUCT(BlueprintType)
struct FEnemyGroupData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Battle")
	EBattleType BattleType;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Battle")
	TArray<TSoftClassPtr<ABattleEnemyCharacter>> EnemyClasses;
};