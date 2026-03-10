#include "TBGameInstance.h"
#include "AbilitySystemGlobals.h"
#include "Engine/DataTable.h"
#include "Math/NumericLimits.h"

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

void UTBGameInstance::AddGold(int32 Amount)
{
	int64 NewValue = (int64)CurrentMoney + Amount;
	CurrentMoney = FMath::Clamp(NewValue, (int64)MIN_int32, (int64)MAX_int32);
}

bool UTBGameInstance::SpendGold(int32 Amount)
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
