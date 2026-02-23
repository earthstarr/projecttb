#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleCombatant.h"
#include "BattleEnemyCharacter.generated.h"

class UTBGameplayAbility;
class UWidgetComponent;
class UEnemyHealthBarWidget;

/**
 * 적 캐릭터 기반 클래스.
 * AI 행동은 SelectAction을 Blueprint에서 오버라이드해 구현.
 * 기본 구현: 살아있는 플레이어 중 랜덤 타겟 + 첫 번째 어빌리티 사용.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTTB_API ABattleEnemyCharacter : public ABattleCombatant
{
	GENERATED_BODY()

public:
	ABattleEnemyCharacter();

	// Blueprint에서 WBP_EnemyHealthBar 클래스를 지정한다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UUserWidget> HealthBarWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="UI")
	TObjectPtr<UWidgetComponent> HealthBarWidgetComponent;

	/**
	 * AI 행동 선택.
	 * Blueprint에서 오버라이드해 적마다 다른 패턴 구현 가능.
	 * @param PlayerParty  살아있는 플레이어 배열
	 * @param OutTarget    선택한 타겟
	 * @param OutAbilityClass 사용할 어빌리티 클래스
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="AI")
	void SelectAction(
		const TArray<ABattleCombatant*>& PlayerParty,
		ABattleCombatant*& OutTarget,
		TSubclassOf<UTBGameplayAbility>& OutAbilityClass);

	virtual void SelectAction_Implementation(
		const TArray<ABattleCombatant*>& PlayerParty,
		ABattleCombatant*& OutTarget,
		TSubclassOf<UTBGameplayAbility>& OutAbilityClass);

	virtual void OnTurnBegin_Implementation() override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnDamageReceivedHandler(ABattleCombatant* Combatant, float Damage);

	void RefreshHealthBar();
};
