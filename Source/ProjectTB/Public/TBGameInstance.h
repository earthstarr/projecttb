#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Data/LevelDataTypes.h"
#include "TBGameInstance.generated.h"

class ABattleEnemyCharacter;
class UDataTable;

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

// 레벨업 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPartyMemberLevelUp, const FLevelUpInfo&, LevelUpInfo);

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

	// ─── 배틀 전환 데이터 ──────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadWrite, Category="Battle")
	FBattleTransitionData BattleTransitionData;

	// ─── 파티 데이터 ──────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadWrite, Category="Party")
	TArray<FPartyMemberData> PartyData;

	// ─── 레벨 스탯 DataTable ──────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Data")
	TSoftObjectPtr<UDataTable> CharacterLevelStatsTable;

	// ─── 델리게이트 ───────────────────────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category="Party")
	FOnPartyMemberLevelUp OnPartyMemberLevelUp;

	// ─── 파티 관리 함수 ───────────────────────────────────────────────────────

	/** 파티 초기화 (게임 시작 시 호출) */
	UFUNCTION(BlueprintCallable, Category="Party")
	void InitializeParty(const TArray<FPartyMemberData>& InitialParty);

	/** 파티원 데이터 조회 (CharacterId로) - C++ 전용 */
	FPartyMemberData* GetPartyMemberData(FName CharacterId);

	/** 파티원 데이터 조회 (CharacterId로) - Blueprint용 */
	UFUNCTION(BlueprintCallable, Category="Party")
	bool GetPartyMemberDataByRef(FName CharacterId, FPartyMemberData& OutData);

	/** 파티원 현재 스탯 저장 (전투 종료 시) */
	UFUNCTION(BlueprintCallable, Category="Party")
	void SavePartyMemberStats(FName CharacterId, float CurrentHP, float CurrentMP, float CurrentStamina);

	// ─── 경험치/레벨 관리 ─────────────────────────────────────────────────────

	/** 파티 전체에 경험치 분배 (전투 승리 시) */
	UFUNCTION(BlueprintCallable, Category="Experience")
	void AddExpToParty(int32 TotalExp);

	/** 개별 파티원에 경험치 추가 + 레벨업 체크 */
	UFUNCTION(BlueprintCallable, Category="Experience")
	void AddExpToMember(FName CharacterId, int32 Exp);

	// ─── 레벨 스탯 조회 ───────────────────────────────────────────────────────

	/** DataTable에서 캐릭터+레벨에 해당하는 스탯 조회 */
	UFUNCTION(BlueprintCallable, Category="Stats")
	bool GetLevelStats(FName CharacterId, int32 Level, FCharacterLevelStats& OutStats);
	
	// ─── 재화 ────────────────────────────────────────────────────────────────
private:
	UPROPERTY(BlueprintReadWrite, Category="Money", META = (AllowPrivateAccess))
	int32 CurrentMoney;
	
public:
	UFUNCTION(BlueprintCallable, Category="Money")
	void AddGold(int32 Amount);
	
	// 소모. 결과가 0원 이상인 경우 소모하며 true 반환. 결과가 음수일 경우 소모하지 않고 false
	UFUNCTION(BlueprintCallable, Category="Money")
	bool SpendGold(int32 Amount);
	
	// 강탈. 언더 플로우 방지 적용됨
	UFUNCTION(BlueprintCallable, Category="Money")
	void RobMoney(int32 Amount);
	
	UFUNCTION(BlueprintCallable, Category="Money")
	int32 GetCurrentMoney() const;
};
