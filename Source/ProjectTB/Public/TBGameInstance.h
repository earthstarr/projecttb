#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TBGameInstance.generated.h"

class ABattleEnemyCharacter;

/** 레벨 간 전투 전환 데이터 */
USTRUCT(BlueprintType)
struct FBattleTransitionData
{
	GENERATED_BODY()

	// 배틀 씬에서 스폰할 적 Blueprint 클래스 목록
	UPROPERTY(BlueprintReadWrite, Category="Battle")
	TArray<TSoftClassPtr<ABattleEnemyCharacter>> EnemyClasses;

	// 전투 종료 후 복귀할 월드 레벨 이름
	UPROPERTY(BlueprintReadWrite, Category="Battle")
	FName WorldLevelName;

	// 복귀 위치/방향
	UPROPERTY(BlueprintReadWrite, Category="Battle")
	FVector  WorldReturnLocation  = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category="Battle")
	FRotator WorldReturnRotation  = FRotator::ZeroRotator;
};

/**
 * GAS 전역 초기화 및 레벨 간 데이터 유지 담당.
 * Project Settings → Maps & Modes → Game Instance Class 에 지정.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTTB_API UTBGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	// 월드 → 배틀 씬 전환 시 여기에 데이터를 저장
	UPROPERTY(BlueprintReadWrite, Category="Battle")
	FBattleTransitionData BattleTransitionData;
};
