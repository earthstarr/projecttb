#include "TBGameInstance.h"
#include "AbilitySystemGlobals.h"
#include "Engine/DataTable.h"
#include "Math/NumericLimits.h"
#include "TBSaveSettings.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"
#include "World/PortalManager.h"

static const FString SettingsSlotName = TEXT("TBSettings");

void UTBGameInstance::Init()
{
	Super::Init();

	// GAS 전역 초기화 — 반드시 한 번 호출해야 ExecutionCalculation 등이 작동함
	UAbilitySystemGlobals::Get().InitGlobalData();

	// 파티 3명 고정 — Blueprint에서 DefaultParty 설정 시 자동 초기화
	if (DefaultParty.Num() > 0)
	{
		PartyData = DefaultParty;
	}

	// 설정 로드 및 적용
	LoadAndApplySettings();
}

// ─── 파티 관리 ─────────────────────────────────────────────────────────────────

void UTBGameInstance::InitializeParty(const TArray<FPartyMemberData>& InitialParty)
{
	PartyData = InitialParty;
}

FPartyMemberData* UTBGameInstance::GetPartyMemberData(FName CharacterId)
{
	for (FPartyMemberData& Member : PartyData)
	{
		if (Member.CharacterId == CharacterId)
			return &Member;
	}
	return nullptr;
}

bool UTBGameInstance::GetPartyMemberDataByRef(FName CharacterId, FPartyMemberData& OutData)
{
	if (FPartyMemberData* Found = GetPartyMemberData(CharacterId))
	{
		OutData = *Found;
		return true;
	}
	return false;
}

void UTBGameInstance::SavePartyMemberStats(FName CharacterId, float CurrentHP, float CurrentMP, float CurrentStamina)
{
	if (FPartyMemberData* Member = GetPartyMemberData(CharacterId))
	{
		Member->CurrentHP = CurrentHP;
		Member->CurrentMP = CurrentMP;
		Member->CurrentStamina = CurrentStamina;
	}
}

// ─── 경험치/레벨 관리 ──────────────────────────────────────────────────────────

void UTBGameInstance::AddExpToParty(int32 TotalExp)
{
	if (PartyData.Num() == 0) return;

	const int32 ExpPerMember = TotalExp / PartyData.Num();

	for (FPartyMemberData& Member : PartyData)
	{
		AddExpToMember(Member.CharacterId, ExpPerMember);
	}
}

void UTBGameInstance::AddExpToMember(FName CharacterId, int32 Exp)
{
	FPartyMemberData* Member = GetPartyMemberData(CharacterId);
	if (!Member) return;

	// 이미 최대 레벨이면 경험치 추가 안함
	if (Member->Level >= FPartyMemberData::MaxLevel)
	{
		return;
	}

	Member->CurrentExp += Exp;

	// 레벨업 체크
	while (Member->Level < FPartyMemberData::MaxLevel)
	{
		const int32 RequiredExp = FPartyMemberData::GetRequiredExpForLevel(Member->Level);
		if (Member->CurrentExp < RequiredExp)
			break;

		// 레벨업
		Member->CurrentExp -= RequiredExp;
		const int32 OldLevel = Member->Level;
		Member->Level++;

		// 레벨업 시 현재 HP/MP/Stamina를 -1로 리셋 (다음 전투에서 풀피로 시작)
		Member->CurrentHP = -1.f;
		Member->CurrentMP = -1.f;
		Member->CurrentStamina = -1.f;

		// 델리게이트 브로드캐스트
		FLevelUpInfo Info;
		Info.CharacterId = CharacterId;
		Info.OldLevel = OldLevel;
		Info.NewLevel = Member->Level;
		OnPartyMemberLevelUp.Broadcast(Info);
	}
}

// ─── 레벨 스탯 조회 ────────────────────────────────────────────────────────────

bool UTBGameInstance::GetLevelStats(FName CharacterId, int32 Level, FCharacterLevelStats& OutStats)
{
	UDataTable* DataTable = CharacterLevelStatsTable.LoadSynchronous();
	if (!DataTable)
	{
		return false;
	}

	// RowName 형식: "CharacterId_Level" (예: "Swordsman_1")
	const FName RowName = FName(*FString::Printf(TEXT("%s_%d"), *CharacterId.ToString(), Level));
	const FCharacterLevelStats* FoundStats = DataTable->FindRow<FCharacterLevelStats>(RowName, TEXT("GetLevelStats"));

	if (FoundStats)
	{
		OutStats = *FoundStats;
		return true;
	}

	return false;
}

// ─── 재화 시스템 ──────────────────────────────────────────────────────────────

void UTBGameInstance::AddMoney(int32 Amount)
{
	int64 NewValue = (int64)CurrentMoney + Amount;
	CurrentMoney = FMath::Clamp(NewValue, (int64)MIN_int32, (int64)MAX_int32);
}

bool UTBGameInstance::SpendMoney(int32 Amount)
{
	if (CurrentMoney < Amount)
	{
		return false;
	}
	else
	{
		CurrentMoney -= Amount;
		return true;
	}
}

void UTBGameInstance::RobMoney(int32 Amount)
{
	int64 NewValue = (int64)CurrentMoney - Amount;
	CurrentMoney = FMath::Clamp(NewValue, (int64)MIN_int32, (int64)MAX_int32);
}

// ─── 주사위 시스템 ──────────────────────────────────────────────────────────────

void UTBGameInstance::AddDice(const FDiceData& NewDice)
{
	OwnedDice.Add(NewDice);
}

FDiceData UTBGameInstance::GetDiceAt(int32 Index) const
{
	if (OwnedDice.IsValidIndex(Index))
		return OwnedDice[Index];
	return FDiceData{};
}

// ─── 아티팩트 시스템 ──────────────────────────────────────────────────────────────

void UTBGameInstance::EquipArtifact(FName ArtifactID)
{
	PartyArtifactData.EquippedArtifact.Add(ArtifactID);
	OnOwnedArtifactsChanged.Broadcast();
}

UDataTable* UTBGameInstance::GetArtifactStatsTable()
{
	if (CachedArtifactStatsTable)
	{
		return CachedArtifactStatsTable;
	}

	CachedArtifactStatsTable = ArtifactStatsTable.LoadSynchronous();
	return CachedArtifactStatsTable;
}

// 아티팩트 데이터 테이블에서 ArtifactID를 검색하여 OutArtifactRow에 전달 
bool UTBGameInstance::GetArtifactRow(FName ArtifactID, FArtifactData& OutArtifactRow)
{
	UDataTable* DataTable = GetArtifactStatsTable();
	if (DataTable == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UTBGameInstance의 GetArtifactRow함수 ArtifactStatsTable.LoadSynchronous 실패"));
		return false;
	}
	
	const FArtifactData* FoundRow = DataTable->FindRow<FArtifactData>(ArtifactID, TEXT("GetArtifactRow"));
	
	if (FoundRow == nullptr)
	{
		return false;
	}
	
	OutArtifactRow = *FoundRow;
	return true;
}

bool UTBGameInstance::HasArtifact(FName ArtifactID) const
{
	return PartyArtifactData.EquippedArtifact.Contains(ArtifactID);
}

TArray<FArtifactEntry> UTBGameInstance::GetUnownedArtifacts()
{
	TArray<FArtifactEntry> Result;

	UDataTable* DataTable = GetArtifactStatsTable();
	if (DataTable == nullptr)
	{
		return Result;
	}

	const TArray<FName> RowNames = DataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		if (HasArtifact(RowName))
		{
			continue;
		}

		const FArtifactData* FoundRow = DataTable->FindRow<FArtifactData>(RowName, TEXT("GetUnownedArtifacts"));
		if (FoundRow == nullptr)
		{
			continue;
		}

		FArtifactEntry Entry;
		Entry.ArtifactID = RowName;
		Entry.ArtifactData = *FoundRow;
		Result.Add(Entry);
	}

	// 보유중이지 않은 아티팩트 데이터와 아이디 구조체 배열 반환
	return Result;
}

// ─── 게임 시작 ──────────────────────────────────────────────────────────────────

void UTBGameInstance::StartNewGame()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("UTBGameInstance::StartNewGame() - World is null"));
		return;
	}

	UPortalManager* PM = World->GetSubsystem<UPortalManager>();
	if (!PM)
	{
		UE_LOG(LogTemp, Warning, TEXT("UTBGameInstance::StartNewGame() - PortalManager subsystem not found"));
		return;
	}

	// LV_Soul_Cave_HubMap으로 이동할 RoomHandle 생성
	FDataTableRowHandle HubRoomHandle;
	HubRoomHandle.DataTable = PM->InitRoomHandle.DataTable; // 같은 DataTable 사용
	HubRoomHandle.RowName = FName("Map_Hub"); // 데이터테이블의 Row 이름

	PM->OnLevelLoadStarted(HubRoomHandle);
}

void UTBGameInstance::QuitGame()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
}

// ─── 설정 시스템 ────────────────────────────────────────────────────────────────

// ===== 설정 저장/로드 =====

void UTBGameInstance::SaveSettings()
{
	if (CachedSettings)
	{
		UGameplayStatics::SaveGameToSlot(CachedSettings, SettingsSlotName, 0);
	}
}

void UTBGameInstance::LoadAndApplySettings()
{
	if (UGameplayStatics::DoesSaveGameExist(SettingsSlotName, 0))
	{
		CachedSettings = Cast<UTBSaveSettings>(UGameplayStatics::LoadGameFromSlot(SettingsSlotName, 0));
	}

	if (!CachedSettings)
	{
		CachedSettings = Cast<UTBSaveSettings>(UGameplayStatics::CreateSaveGameObject(UTBSaveSettings::StaticClass()));
	}

	// Audio 볼륨 적용
	if (MasterSoundMix)
	{
		UGameplayStatics::PushSoundMixModifier(GetWorld(), MasterSoundMix);
		ApplyAudioVolume(MasterSoundClass, CachedSettings->MasterVolume);
		ApplyAudioVolume(MusicSoundClass, CachedSettings->MusicVolume);
		ApplyAudioVolume(SFXSoundClass, CachedSettings->SFXVolume);
	}

	// Brightness 적용
	if (GEngine)
	{
		float Gamma = FMath::GetMappedRangeValueClamped(
			FVector2D(0.0f, 100.0f), FVector2D(1.5f, 3.0f), CachedSettings->Brightness);
		GEngine->DisplayGamma = Gamma;
	}
}

// ===== Audio 설정 =====

void UTBGameInstance::ApplyAudioVolume(USoundClass* SoundClass, float Volume)
{
	if (!MasterSoundMix || !SoundClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	UGameplayStatics::SetSoundMixClassOverride(World, MasterSoundMix, SoundClass,
		FMath::Clamp(Volume, 0.0f, 1.0f), 1.0f, 0.0f, true);
}

void UTBGameInstance::SetMasterVolume(float Volume)
{
	if (!CachedSettings) return;
	CachedSettings->MasterVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyAudioVolume(MasterSoundClass, CachedSettings->MasterVolume);
	SaveSettings();
}

float UTBGameInstance::GetMasterVolume() const
{
	return CachedSettings ? CachedSettings->MasterVolume : 1.0f;
}

void UTBGameInstance::SetMusicVolume(float Volume)
{
	if (!CachedSettings) return;
	CachedSettings->MusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyAudioVolume(MusicSoundClass, CachedSettings->MusicVolume);
	SaveSettings();
}

float UTBGameInstance::GetMusicVolume() const
{
	return CachedSettings ? CachedSettings->MusicVolume : 1.0f;
}

void UTBGameInstance::SetSFXVolume(float Volume)
{
	if (!CachedSettings) return;
	CachedSettings->SFXVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyAudioVolume(SFXSoundClass, CachedSettings->SFXVolume);
	SaveSettings();
}

float UTBGameInstance::GetSFXVolume() const
{
	return CachedSettings ? CachedSettings->SFXVolume : 1.0f;
}

// ===== Video 설정 =====

void UTBGameInstance::SetWindowMode(int32 Mode)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings) return;

	EWindowMode::Type WinMode;
	switch (Mode)
	{
	case 0: WinMode = EWindowMode::Fullscreen; break;
	case 1: WinMode = EWindowMode::WindowedFullscreen; break;
	case 2: WinMode = EWindowMode::Windowed; break;
	default: WinMode = EWindowMode::WindowedFullscreen; break;
	}

	Settings->SetFullscreenMode(WinMode);

	// Fullscreen 모드는 데스크탑 해상도와 일치해야 정상 작동
	if (WinMode == EWindowMode::Fullscreen)
	{
		Settings->SetScreenResolution(Settings->GetDesktopResolution());
	}
}

int32 UTBGameInstance::GetWindowMode() const
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings) return 1;

	switch (Settings->GetFullscreenMode())
	{
	case EWindowMode::Fullscreen: return 0;
	case EWindowMode::WindowedFullscreen: return 1;
	case EWindowMode::Windowed: return 2;
	default: return 1;
	}
}

TArray<FIntPoint> UTBGameInstance::GetSupportedResolutions() const
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	FIntPoint DesktopRes = Settings ? Settings->GetDesktopResolution() : FIntPoint(1920, 1080);

	// 고정 해상도 목록 (16:9, 큰 순서)
	const TArray<FIntPoint> AllResolutions = {
		FIntPoint(3840, 2160),
		FIntPoint(2560, 1440),
		FIntPoint(1920, 1080),
		FIntPoint(1600, 900),
		FIntPoint(1280, 720)
	};

	TArray<FIntPoint> Resolutions;
	for (const FIntPoint& Res : AllResolutions)
	{
		if (Res.X <= DesktopRes.X && Res.Y <= DesktopRes.Y)
		{
			Resolutions.Add(Res);
		}
	}

	return Resolutions;
}

void UTBGameInstance::SetResolution(FIntPoint Resolution)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings) return;

	Settings->SetScreenResolution(Resolution);
}

FIntPoint UTBGameInstance::GetCurrentResolution() const
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings) return FIntPoint(1920, 1080);

	return Settings->GetScreenResolution();
}

FString UTBGameInstance::FormatResolution(FIntPoint Resolution)
{
	return FString::Printf(TEXT("%d x %d"), Resolution.X, Resolution.Y);
}

void UTBGameInstance::SetBrightness(float Value)
{
	if (!CachedSettings) return;
	CachedSettings->Brightness = FMath::Clamp(Value, 0.0f, 100.0f);

	if (GEngine)
	{
		float Gamma = FMath::GetMappedRangeValueClamped(
			FVector2D(0.0f, 100.0f), FVector2D(1.5f, 3.0f), CachedSettings->Brightness);
		GEngine->DisplayGamma = Gamma;
	}

	SaveSettings();
}

float UTBGameInstance::GetBrightness() const
{
	return CachedSettings ? CachedSettings->Brightness : 50.0f;
}

void UTBGameInstance::SetOverallQuality(int32 Level)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings) return;
	Settings->SetOverallScalabilityLevel(Level);
}

int32 UTBGameInstance::GetOverallQuality() const
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	return Settings ? Settings->GetOverallScalabilityLevel() : 3;
}

void UTBGameInstance::SetShadowQuality(int32 Level)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings) Settings->SetShadowQuality(Level);
}

int32 UTBGameInstance::GetShadowQuality() const
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	return Settings ? Settings->GetShadowQuality() : 3;
}

void UTBGameInstance::SetAntiAliasingQuality(int32 Level)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings) Settings->SetAntiAliasingQuality(Level);
}

int32 UTBGameInstance::GetAntiAliasingQuality() const
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	return Settings ? Settings->GetAntiAliasingQuality() : 3;
}

void UTBGameInstance::SetTextureQuality(int32 Level)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings) Settings->SetTextureQuality(Level);
}

int32 UTBGameInstance::GetTextureQuality() const
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	return Settings ? Settings->GetTextureQuality() : 3;
}

void UTBGameInstance::SetViewDistanceQuality(int32 Level)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings) Settings->SetViewDistanceQuality(Level);
}

int32 UTBGameInstance::GetViewDistanceQuality() const
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	return Settings ? Settings->GetViewDistanceQuality() : 3;
}

void UTBGameInstance::SetEffectsQuality(int32 Level)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings) Settings->SetVisualEffectQuality(Level);
}

int32 UTBGameInstance::GetEffectsQuality() const
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	return Settings ? Settings->GetVisualEffectQuality() : 3;
}

void UTBGameInstance::ApplyVideoSettings()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings) return;

	Settings->ApplySettings(false);
	Settings->SaveSettings();
}
