#include "Abilities/AnimNotify_OnHit.h"
#include "Battle/BattleCombatant.h"

void UAnimNotify_OnHit::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	if (ABattleCombatant* Combatant = Cast<ABattleCombatant>(MeshComp->GetOwner()))
		Combatant->OnHitNotify(HitIndex);
}
