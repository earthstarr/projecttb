#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RoomDataStruct.generated.h"


UENUM(BlueprintType)
enum class ERoomType : uint8
{
	Test,
	Battle,
	World,
	Event,
	Shop
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
};
