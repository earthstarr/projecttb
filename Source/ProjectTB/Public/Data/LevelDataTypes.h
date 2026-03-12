#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LevelDataTypes.generated.h"

class UTBGameplayAbility;

// ─── DataTable 행: 캐릭터별 레벨 스탯 ─────────────────────────────────────────
USTRUCT(BlueprintType)
struct FCharacterLevelStats : public FTableRowBase
{
	GENERATED_BODY()

	// 캐릭터 식별자 ("Swordsman", "Mage", "Archer")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level")
	FName CharacterId;

	// 레벨 (1~10)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level")
	int32 Level = 1;

	// ─── Max 스탯 ─────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float MaxHP = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float MaxMP = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float MaxStamina = 100.f;

	// ─── 전투 스탯 ────────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float PhysicalAttack = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float MagicAttack = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float PhysicalDefense = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float MagicDefense = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float Speed = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float CriticalChance = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float CriticalMultiplier = 1.5f;
};

// ─── GameInstance 저장용: 파티원 런타임 데이터 ────────────────────────────────
USTRUCT(BlueprintType)
struct FPartyMemberData
{
	GENERATED_BODY()

	// 캐릭터 식별자 (DataTable 조회용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Party")
	FName CharacterId;

	// 스폰할 Blueprint 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Party")
	TSubclassOf<class ABattlePlayerCharacter> CharacterClass;

	// ─── 레벨/경험치 ──────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level")
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level")
	int32 CurrentExp = 0;

	// ─── 현재 스탯 (전투 간 유지) ──────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float CurrentHP = -1.f;  // -1 = 풀피로 시작

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float CurrentMP = -1.f;  // -1 = 풀마나로 시작

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float CurrentStamina = -1.f;  // -1 = 풀스태미나로 시작

	// ─── 장착 주사위 ─────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dice")
	int32 EquippedDiceIndex = 0;  // GameInstance.OwnedDice 인덱스

	// ─── 유틸 함수 ────────────────────────────────────────────────────────────

	// 해당 레벨에서 다음 레벨까지 필요한 경험치
	static int32 GetRequiredExpForLevel(int32 InLevel)
	{
		return InLevel * 100;  // 1→2: 100, 2→3: 200, ...
	}

	// 최대 레벨
	static constexpr int32 MaxLevel = 10;
};

// ─── 레벨업 정보 (델리게이트 전달용) ───────────────────────────────────────────
USTRUCT(BlueprintType)
struct FLevelUpInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName CharacterId;

	UPROPERTY(BlueprintReadOnly)
	int32 OldLevel = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 NewLevel = 0;
};

// ─── 캐릭터별 경험치 표시용 데이터 ────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FCharacterExpData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Exp")
	FName CharacterId;

	UPROPERTY(BlueprintReadOnly, Category="Exp")
	int32 Level = 1;

	UPROPERTY(BlueprintReadOnly, Category="Exp")
	int32 CurrentExp = 0;

	UPROPERTY(BlueprintReadOnly, Category="Exp")
	int32 ExpToNextLevel = 100;

	/** 이번 전투에서 획득한 경험치 */
	UPROPERTY(BlueprintReadOnly, Category="Exp")
	int32 GainedExp = 0;

	UPROPERTY(BlueprintReadOnly, Category="Exp")
	bool bLeveledUp = false;
};

// ─── 레벨업 UI 표시용 데이터 ──────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FLevelUpDisplayData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="LevelUp")
	FName CharacterId;

	UPROPERTY(BlueprintReadOnly, Category="LevelUp")
	int32 OldLevel = 0;

	UPROPERTY(BlueprintReadOnly, Category="LevelUp")
	int32 NewLevel = 0;

	UPROPERTY(BlueprintReadOnly, Category="LevelUp")
	FCharacterLevelStats OldStats;

	UPROPERTY(BlueprintReadOnly, Category="LevelUp")
	FCharacterLevelStats NewStats;

	UPROPERTY(BlueprintReadOnly, Category="LevelUp")
	bool bLeveledUp = false;

	/** 이번 레벨업으로 해금된 스킬 목록 */
	UPROPERTY(BlueprintReadOnly, Category="LevelUp")
	TArray<TSubclassOf<UTBGameplayAbility>> UnlockedAbilities;
};
