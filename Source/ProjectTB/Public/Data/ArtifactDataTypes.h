// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayEffectTypes.h"
#include "AttributeSet.h"
#include "Engine//DataTable.h"
#include "ArtifactDataTypes.generated.h"

class AArtifactBase;

// 적용할 대상
UENUM(BlueprintType)
enum class EArtifactTargetMode : uint8
{
	AllParty,
	MatchingJob,
};

// ─── 스탯 변경 아이템 데이터 테이블의 행 ────────────────────────────────
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

// ─── 스탯 변경 아이템 데이터 테이블의 원소 ────────────────────────────────
USTRUCT(BlueprintType)
struct FArtifact_CharacterStats : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EArtifactTargetMode TargetMode = EArtifactTargetMode::MatchingJob;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> TargetJobIds;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FArtifactStatModifier> StatModifiers;
};

// ─── GameInstance 저장용: 파티원 보유 아이템 데이터 ────────────────────────────────
USTRUCT(BlueprintType)
struct FEquippedArtifactData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Artifact")
	TSet<FName> EquippedArtifact;
	//TMap<FName, FActiveGameplayEffectHandle> EquippedArtifact;
};