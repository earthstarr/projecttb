#include "Battle/BattlePlayerCharacter.h"
#include "TBGameInstance.h"

ABattlePlayerCharacter::ABattlePlayerCharacter() {}

// ─── 주사위 ─────────────────────────────────────────────────────────────────────

FDiceData ABattlePlayerCharacter::GetEquippedDice() const
{
	UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance());
	if (!GI) return FDiceData{};

	// PartyData에서 현재 캐릭터의 EquippedDiceIndex 조회
	for (const FPartyMemberData& Member : GI->PartyData)
	{
		if (Member.CharacterId == CharacterId)
		{
			return GI->GetDiceAt(Member.EquippedDiceIndex);
		}
	}
	return FDiceData{};
}

void ABattlePlayerCharacter::EquipDice(int32 DiceIndex)
{
	UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance());
	if (!GI) return;

	// PartyData에서 현재 캐릭터의 EquippedDiceIndex 업데이트
	for (FPartyMemberData& Member : GI->PartyData)
	{
		if (Member.CharacterId == CharacterId)
		{
			if (GI->OwnedDice.IsValidIndex(DiceIndex))
			{
				Member.EquippedDiceIndex = DiceIndex;
			}
			return;
		}
	}
}

int32 ABattlePlayerCharacter::GetEquippedDiceIndex() const
{
	UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance());
	if (!GI) return 0;

	for (const FPartyMemberData& Member : GI->PartyData)
	{
		if (Member.CharacterId == CharacterId)
		{
			return Member.EquippedDiceIndex;
		}
	}
	return 0;
}

void ABattlePlayerCharacter::OnTurnBegin_Implementation()
{
	Super::OnTurnBegin_Implementation();
	// BattleManager가 BattleMenuWidget을 열어 입력 대기
}

void ABattlePlayerCharacter::OnTurnEnd_Implementation()
{
	Super::OnTurnEnd_Implementation();
}
