#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StatusIconPanelWidget.generated.h"

class ABattleCombatant;

UCLASS()
class PROJECTTB_API UStatusIconPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// C++에서 델리게이트 바인딩까지 처리 → Blueprint에서 그냥 호출만 하면 됨
	UFUNCTION(BlueprintCallable, Category="StatusEffect")
	void InitWithCombatant(ABattleCombatant* Combatant);

	// Blueprint(WBP_StatusIconPanel)에서 구현: 아이콘 목록 갱신
	UFUNCTION(BlueprintImplementableEvent, Category="StatusEffect")
	void RefreshIcons(ABattleCombatant* Combatant);

private:
	UFUNCTION()
	void OnStatusEffectsChangedHandler(ABattleCombatant* Combatant);

	TWeakObjectPtr<ABattleCombatant> OwnerCombatant;
};
