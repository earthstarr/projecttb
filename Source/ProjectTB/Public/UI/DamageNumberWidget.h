#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageNumberWidget.generated.h"

class UTextBlock;

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
	void SetDamage(float Damage);

protected:
	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	TObjectPtr<UTextBlock> DamageText;
};
