#include "Abilities/AnimNotify_OpenParryTiming.h"
#include "Battle/BattleCombatant.h"

void UAnimNotify_OpenParryTiming::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	if (ABattleCombatant* Combatant = Cast<ABattleCombatant>(MeshComp->GetOwner()))
		Combatant->OnOpenParryTimingNotify(Duration);
}
