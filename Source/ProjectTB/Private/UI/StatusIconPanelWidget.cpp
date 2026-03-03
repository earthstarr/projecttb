#include "UI/StatusIconPanelWidget.h"
#include "Battle/BattleCombatant.h"

void UStatusIconPanelWidget::InitWithCombatant(ABattleCombatant* Combatant)
{
	if (!Combatant) return;

	OwnerCombatant = Combatant;
	Combatant->OnStatusEffectsChanged.AddDynamic(this, &UStatusIconPanelWidget::OnStatusEffectsChangedHandler);

	// 초기 상태 즉시 반영
	RefreshIcons(Combatant);
}

void UStatusIconPanelWidget::OnStatusEffectsChangedHandler(ABattleCombatant* Combatant)
{
	RefreshIcons(Combatant);
}
