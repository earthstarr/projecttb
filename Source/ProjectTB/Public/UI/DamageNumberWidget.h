#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageNumberWidget.generated.h"

class UTextBlock;
class AActor;

DECLARE_DELEGATE(FOnDamageNumberFinished);

/**
 * 데미지 숫자 위젯 기반 클래스.
 * WBP_DamageNumber가 이 클래스를 부모로 사용한다.
 * TextBlock 이름을 "DamageText"로 맞춰야 BindWidget이 동작한다.
 */
UCLASS()
class PROJECTTB_API UDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetDamage(float Damage, bool bIsCritical = false);
	void SetHeal(float Heal);

	// 스택 시스템: 위로 이동 애니메이션
	void MoveUp(float Distance);

	// 위젯 소멸 시 호출되는 델리게이트
	FOnDamageNumberFinished OnFinished;

	// 수명 시작 (타이머로 자동 제거)
	void StartLifespan(float Duration = 2.0f);

	// 따라갈 타겟 액터 설정
	void SetFollowTarget(AActor* InTarget, float InHeightOffset = 100.f);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	TObjectPtr<UTextBlock> DamageText;

private:
	// 따라갈 타겟
	TWeakObjectPtr<AActor> FollowTarget;
	float HeightOffset = 100.f;
	void UpdateScreenPosition();

	// 스택 오프셋 (위로 이동 애니메이션)
	float TargetOffsetY = 0.f;
	float CurrentOffsetY = 0.f;

	// 수명 관리
	FTimerHandle LifespanTimer;
	void OnLifespanExpired();

	// 페이드 아웃
	float FadeOutDuration = 0.5f;
	float FadeOutTimer = 0.f;
	bool bFadingOut = false;
};
