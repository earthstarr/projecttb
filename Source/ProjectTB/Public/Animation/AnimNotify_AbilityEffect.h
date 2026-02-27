#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_AbilityEffect.generated.h"

/**
 * 어빌리티 몽타주에서 HitEffect를 스폰하는 AnimNotify.
 * TBGameplayAbility의 HitEffect가 설정되어 있으면 시전자 위치에 스폰한다.
 */
UCLASS(DisplayName="Ability Effect")
class PROJECTTB_API UAnimNotify_AbilityEffect : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override { return TEXT("Ability Effect"); }
};
