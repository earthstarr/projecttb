#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterStatusWidget.generated.h"

class ABattleCombatant;
class ABattlePlayerCharacter;

/**
 * 하단 캐릭터 스탯 바 (HP/MP/Stamina).
 * AttributeSet 변경 시 OnDamageReceived 델리게이트를 통해 갱신.
 * 3개 슬롯 레이아웃과 바 애니메이션은 Blueprint에서 구현.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTTB_API UCharacterStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 전투 시작 시 파티 정보로 초기화
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Status")
	void InitializeParty(const TArray<ABattlePlayerCharacter*>& Party);
	virtual void InitializeParty_Implementation(const TArray<ABattlePlayerCharacter*>& Party) {}

	// 현재 턴 캐릭터 하이라이트
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Status")
	void SetActiveCharacter(ABattleCombatant* ActiveCombatant);
	virtual void SetActiveCharacter_Implementation(ABattleCombatant* ActiveCombatant) {}

	// 특정 캐릭터 스탯 바 갱신
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Status")
	void RefreshStatus(ABattleCombatant* Combatant);
	virtual void RefreshStatus_Implementation(ABattleCombatant* Combatant) {}
};
