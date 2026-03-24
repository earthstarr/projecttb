#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Data/LevelDataTypes.h"
#include "Data/ArtifactDataTypes.h"
#include "Battle/TBDiceData.h"
#include "TBGameInstance.generated.h"

class UPortalSpawnConfig;
class ABattleEnemyCharacter;
class UDataTable;
class USoundMix;
class USoundClass;
class UTBSaveSettings;

/** 레벨 간 전투 전환 데이터 */
USTRUCT(BlueprintType)
struct FBattleTransitionData
{
	GENERATED_BODY()

	// 배틀 씬에서 스폰할 적 Blueprint 클래스 목록
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Battle")
	TArray<TSoftClassPtr<ABattleEnemyCharacter>> EnemyClasses;

	// 전투 종료 후 복귀할 월드 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Battle")
	FDataTableRowHandle PostBattleRoomData;
};

// 레벨업 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPartyMemberLevelUp, const FLevelUpInfo&, LevelUpInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOwnedArtifactsChanged);

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

	/** Blueprint에서 기본 파티 3명 설정 — Init() 시 PartyData로 자동 복사 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Party")
	TArray<FPartyMemberData> DefaultParty;

	UPROPERTY(BlueprintReadWrite, Category="Party")
	TArray<FPartyMemberData> PartyData;

	// ─── 레벨 스탯 DataTable ──────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Data")
	TSoftObjectPtr<UDataTable> CharacterLevelStatsTable;

	// ─── 델리게이트 ───────────────────────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category="Party")
	FOnPartyMemberLevelUp OnPartyMemberLevelUp;

	UPROPERTY(BlueprintAssignable, Category="Artifact")
	FOnOwnedArtifactsChanged OnOwnedArtifactsChanged;

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
	
	// ─── 재화 ───────────────────────────────────────────────────────────────
#pragma region Money
private:
	UPROPERTY(BlueprintReadWrite, Category="Money", META = (AllowPrivateAccess))
	int32 CurrentMoney;
	
public:
	UFUNCTION(BlueprintCallable, Category="Money")
	void AddMoney(int32 Amount);
	
	// 소모. 결과가 0원 이상인 경우 소모하며 true 반환. 결과가 음수일 경우 소모하지 않고 false
	UFUNCTION(BlueprintCallable, Category="Money")
	bool SpendMoney(int32 Amount);
	
	// 강탈. 언더 플로우 방지 적용됨
	UFUNCTION(BlueprintCallable, Category="Money")
	void RobMoney(int32 Amount);
	
	UFUNCTION(BlueprintCallable, Category="Money")
	int32 GetCurrentMoney() const {return CurrentMoney;}

#pragma endregion

	// ─── 주사위 ───────────────────────────────────────────────────────────────

	// 파티 공용 주사위 인벤토리
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Dice")
	TArray<FDiceData> OwnedDice;

	// 유물로 쌓이는 파티 전체 주사위 보정
	UPROPERTY(BlueprintReadWrite, Category="Dice")
	FDiceModifier DiceModifier;

	// 주사위 획득
	UFUNCTION(BlueprintCallable, Category="Dice")
	void AddDice(const FDiceData& NewDice);

	// 인덱스로 주사위 조회 (범위 초과 시 빈 주사위 반환)
	UFUNCTION(BlueprintCallable, Category="Dice")
	FDiceData GetDiceAt(int32 Index) const;
	
	// ─── 아티팩트 ─────────────────────────────────────────────────────────────
#pragma region Artifacts
	// 아티팩트 데이터 테이블
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Artifact")
	TSoftObjectPtr<UDataTable> ArtifactStatsTable;

	UPROPERTY(Transient)
	TObjectPtr<UDataTable> CachedArtifactStatsTable;
	
	// 파티 보유 아티팩트 데이터 (이름만)
	UPROPERTY(BlueprintReadWrite, Category="Artifact")
	FEquippedArtifactData PartyArtifactData;
	
	// 최대 보유 아티팩트 갯수 한도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Artifact")
	int32 MaxArtifactCount = 100;
	
	// 아티팩트 습득
	UFUNCTION(BlueprintCallable, Category="Artifact")
	void EquipArtifact(FName ArtifactID);
	
	// 아티팩트 데이터 테이블 캐싱 함수
	UDataTable* GetArtifactStatsTable();
	
	// 아티팩트 데이터 테이블 조회 결과 래퍼 함수
	UFUNCTION(BlueprintCallable, Category="Artifact")
	bool GetArtifactRow(FName ArtifactID, FArtifactData& OutArtifactRow);
	
	// 대상 아티팩트 보유 여부 체크
	UFUNCTION(BlueprintCallable, Category="Artifact")
	bool HasArtifact(FName ArtifactID) const;

	// <미보유 아티팩트 아이디 + 데이터 테이블> 구조체 배열 반환 함수
	UFUNCTION(BlueprintCallable, Category="Artifact")
	TArray<FArtifactEntry> GetUnownedArtifacts();
#pragma endregion
	
#pragma region Portal
private:
	UPROPERTY(BlueprintReadWrite, Category="Portal", meta=(AllowPrivateAccess="true"))
	int32 PortalMoveCount = 0;
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Portal")
	TSoftObjectPtr<UPortalSpawnConfig> PortalSpawnConfig;
	
	UFUNCTION(BlueprintCallable, Category="Portal")
	void ResetPortalMoveCount() { PortalMoveCount = 0; }

	UFUNCTION(BlueprintCallable, Category="Portal")
	void SetPortalMoveCount(int32 NewCount)
	{
		PortalMoveCount = FMath::Max(NewCount, 0);
	}
	
	UFUNCTION(BlueprintCallable, Category="Portal")
	void IncreasePortalMoveCount() { ++PortalMoveCount; }

	UFUNCTION(BlueprintCallable, Category="Portal")
	void DecreasePortalMoveCount()
	{
		PortalMoveCount = FMath::Max(PortalMoveCount - 1, 0);
	}
	
	UFUNCTION(BlueprintCallable, Category="Portal")
	int32 GetPortalMoveCount() const { return PortalMoveCount; }
#pragma endregion

	// ─── 게임 시작 ────────────────────────────────────────────────────────────

	/** 새 게임 시작 (메인 메뉴에서 호출) */
	UFUNCTION(BlueprintCallable, Category="Game")
	void StartNewGame();

	/** 게임 종료 (메인 메뉴에서 호출) */
	UFUNCTION(BlueprintCallable, Category="Game")
	void QuitGame();

	/** 메인 메뉴 카메라 애니메이션 토글 (매번 번갈아가며 실행) */
	UPROPERTY(BlueprintReadWrite, Category="Game")
	bool bUseAlternateCameraAnimation = false;

	// ─── 설정 시스템 ──────────────────────────────────────────────────────────

	// --- Audio 설정 ---
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMasterVolume(float Volume);

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMasterVolume() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMusicVolume(float Volume);

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMusicVolume() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetSFXVolume(float Volume);

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetSFXVolume() const;

	// --- Video 설정 ---
	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetWindowMode(int32 Mode);

	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	int32 GetWindowMode() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	TArray<FIntPoint> GetSupportedResolutions() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetResolution(FIntPoint Resolution);

	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	FIntPoint GetCurrentResolution() const;

	// "1920 x 1080" 형식으로 반환
	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	static FString FormatResolution(FIntPoint Resolution);

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetBrightness(float Value);

	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	float GetBrightness() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetOverallQuality(int32 Level);

	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	int32 GetOverallQuality() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetShadowQuality(int32 Level);

	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	int32 GetShadowQuality() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetAntiAliasingQuality(int32 Level);

	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	int32 GetAntiAliasingQuality() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetTextureQuality(int32 Level);

	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	int32 GetTextureQuality() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetViewDistanceQuality(int32 Level);

	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	int32 GetViewDistanceQuality() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetEffectsQuality(int32 Level);

	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	int32 GetEffectsQuality() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void ApplyVideoSettings();

	// --- 저장/로드 ---
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SaveSettings();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void LoadAndApplySettings();

	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	UTBSaveSettings* CachedSettings;

protected:
	// === 설정용 에셋 (에디터에서 할당) ===
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Audio")
	USoundMix* MasterSoundMix;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Audio")
	USoundClass* MasterSoundClass;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Audio")
	USoundClass* MusicSoundClass;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Audio")
	USoundClass* SFXSoundClass;

	void ApplyAudioVolume(USoundClass* SoundClass, float Volume);
};
