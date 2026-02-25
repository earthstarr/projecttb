#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_OnHit.generated.h"

/**
 * 몽타주 특정 프레임에 배치해 타격 데미지 타이밍을 제어하는 Notify.
 * HitIndex → TBGameplayAbility::HitMultipliers 배열 인덱스에 대응.
 *
 * 사용법: 몽타주 에디터 → Notifies 트랙 → 우클릭 → Add Notify → AnimNotify_OnHit
 * 단타: HitIndex=0 / 연타: 각 타격마다 HitIndex=0,1,2...
 */
UCLASS(meta=(DisplayName="On Hit (Battle)"))
class PROJECTTB_API UAnimNotify_OnHit : public UAnimNotify
{
	GENERATED_BODY()

public:
	// 몽타주 에디터에서 타격별로 다르게 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Battle")
	int32 HitIndex = 0;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
