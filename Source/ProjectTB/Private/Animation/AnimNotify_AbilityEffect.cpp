#include "Animation/AnimNotify_AbilityEffect.h"
#include "Battle/BattleCombatant.h"
#include "AbilitySystemComponent.h"
#include "Abilities/TBGameplayAbility.h"

void UAnimNotify_AbilityEffect::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	AActor* Owner = MeshComp->GetOwner();
	ABattleCombatant* Combatant = Cast<ABattleCombatant>(Owner);
	if (!Combatant) return;

	UAbilitySystemComponent* ASC = Combatant->GetAbilitySystemComponent();
	if (!ASC) return;

	// 현재 활성화된 TBGameplayAbility 찾기
	TArray<FGameplayAbilitySpec>& Specs = ASC->GetActivatableAbilities();
	for (FGameplayAbilitySpec& Spec : Specs)
	{
		if (!Spec.IsActive()) continue;

		UTBGameplayAbility* ActiveAbility = Cast<UTBGameplayAbility>(Spec.GetPrimaryInstance());
		if (ActiveAbility)
		{
			ActiveAbility->SpawnHitEffect();
			break;
		}
	}
}
