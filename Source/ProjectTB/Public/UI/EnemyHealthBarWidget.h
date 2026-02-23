#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyHealthBarWidget.generated.h"

class UProgressBar;

/**
 * 적 머리 위에 표시되는 체력바 위젯 기반 클래스.
 * WBP_EnemyHealthBar가 이 클래스를 부모로 사용한다.
 * ProgressBar 이름을 "HealthBar"로 맞춰야 BindWidget이 동작한다.
 */
UCLASS()
class PROJECTTB_API UEnemyHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void UpdateHealth(float CurrentHP, float MaxHP);

protected:
	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	TObjectPtr<UProgressBar> HealthBar;
};
