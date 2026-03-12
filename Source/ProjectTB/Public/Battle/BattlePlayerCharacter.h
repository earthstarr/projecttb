#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleCombatant.h"
#include "Battle/TBDiceData.h"
#include "BattlePlayerCharacter.generated.h"

/**
 * 플레이어 파티 캐릭터.
 * Blueprint에서 검사/마법사/궁수로 서브클래싱.
 * 메시, 애니메이션, StartingEffects/Abilities는 Blueprint에서 설정.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTTB_API ABattlePlayerCharacter : public ABattleCombatant
{
	GENERATED_BODY()

public:
	ABattlePlayerCharacter();

	// 파티 내 인덱스 (0, 1, 2) — 하단 StatusWidget 슬롯에 대응
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Party")
	int32 PartyIndex = 0;

	// GameInstance 파티 데이터와 연결하기 위한 식별자 ("Swordsman", "Mage", "Archer")
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Party")
	FName CharacterId;

	// 캐릭터 레벨 (스폰 시 GameInstance에서 복사)
	UPROPERTY(BlueprintReadOnly, Category="Party")
	int32 CharacterLevel = 1;

	// ─── 주사위 ───────────────────────────────────────────────────────────────

	// GameInstance.OwnedDice에서 장착된 주사위 반환
	UFUNCTION(BlueprintCallable, Category="Dice")
	FDiceData GetEquippedDice() const;

	// 주사위 장착 (GameInstance.PartyData.EquippedDiceIndex 업데이트)
	UFUNCTION(BlueprintCallable, Category="Dice")
	void EquipDice(int32 DiceIndex);

	// 현재 장착된 주사위 인덱스 조회
	UFUNCTION(BlueprintCallable, Category="Dice")
	int32 GetEquippedDiceIndex() const;

	virtual void OnTurnBegin_Implementation() override;
	virtual void OnTurnEnd_Implementation() override;
	
	
	UFUNCTION(BlueprintCallable, Category="Attributes") 
	FDiceModifier GetCurrentDiceModifier() const;
	
};
