#include "Abilities/AnimNotify_SpawnImpact.h"
#include "Battle/BattleCombatant.h"

void UAnimNotify_SpawnImpact::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (ABattleCombatant* Combatant = Cast<ABattleCombatant>(MeshComp->GetOwner()))
	{
		// 전투원에게 임팩트 액터 스폰 요청
		Combatant->OnSpawnImpactNotify(HitIndex);
	}
}
