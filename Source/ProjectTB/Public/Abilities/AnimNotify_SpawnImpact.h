#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_SpawnImpact.generated.h"

UCLASS(meta=(DisplayName="Spawn Impact (Battle)"))
class PROJECTTB_API UAnimNotify_SpawnImpact : public UAnimNotify
{
	GENERATED_BODY()

public:
	// HitMultipliers 배열 인덱스 (OnHit과 동일)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Battle")
	int32 HitIndex = 0;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};