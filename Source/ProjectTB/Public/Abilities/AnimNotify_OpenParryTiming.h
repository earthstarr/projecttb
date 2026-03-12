#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_OpenParryTiming.generated.h"

/**
 * 적 공격 몽타주에서 패링 타이밍을 여는 Notify.
 * OnHit 노티파이 직전에 배치하여 플레이어가 패링할 수 있는 시간을 부여한다.
 *
 * 사용법: 몽타주 에디터 → Notifies 트랙 → Add Notify → AnimNotify_OpenParryTiming
 *        OnHit 노티파이 앞에 Duration만큼 여유를 두고 배치
 */
UCLASS(meta=(DisplayName="Open Parry Timing (Battle)"))
class PROJECTTB_API UAnimNotify_OpenParryTiming : public UAnimNotify
{
	GENERATED_BODY()

public:
	// 패링 입력을 받을 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Battle|Parry")
	float Duration = 0.6f;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
