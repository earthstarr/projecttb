// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayEffectTypes.h"
#include "AttributeSet.h"
#include "Battle/TBDiceData.h"
#include "Engine//DataTable.h"
#include "GameplayTagContainer.h"
#include "ArtifactDataTypes.generated.h"

class AArtifactBase;

// 적용할 대상
UENUM(BlueprintType)
enum class EArtifactTargetMode : uint8
{
	AllParty,
	MatchingJob,
};

// 아티팩트 등급
UENUM(BlueprintType)
enum class EArtifactGrade : uint8
{
	Common,
	Rare,
	Epic,
	Legendary,
	Boss	// 보스전 보상으로만 획득 가능
};

// ─── 데이터 테이블의 아티팩트 GE 설정 ────────────────────────────────
USTRUCT(BlueprintType)
struct FArtifactStatModifier
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayAttribute Attribute;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EGameplayModOp::Type> ModifierOp = EGameplayModOp::Additive;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value;
};

// ─── 아티팩트 데이터 테이블의 행 ────────────────────────────────
USTRUCT(BlueprintType)
struct FArtifactData : public FTableRowBase
{
	GENERATED_BODY()
	
	// 상점 / 보유 목록 / 상세 UI 표시용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Artifact|Display")
	FText DisplayName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Artifact|Display")
	TSoftObjectPtr<UTexture2D> Icon;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Artifact|Display")
	EArtifactGrade Grade = EArtifactGrade::Common;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Artifact|Display")
	FText Description;
	
	// 상점 구매 정보
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Artifact|Shop", meta=(ClampMin="0"))
	int32 Price = 0;
	
	// 전투 적용 정보
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EArtifactTargetMode TargetMode = EArtifactTargetMode::MatchingJob;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> TargetJobIds;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FArtifactStatModifier> StatModifiers;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer Traits;
};

// ─── GameInstance 저장용: 파티원 보유 아이템 데이터 ────────────────────────────────
USTRUCT(BlueprintType)
struct FEquippedArtifactData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Artifact")
	TSet<FName> EquippedArtifact;
};

// 아티팩트 ID, 데이터 묶음 반환용 구조체
USTRUCT(BlueprintType)
struct FArtifactEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Artifact")
	FName ArtifactID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Artifact")
	FArtifactData ArtifactData;
};