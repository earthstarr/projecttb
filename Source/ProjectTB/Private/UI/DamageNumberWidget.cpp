#include "UI/DamageNumberWidget.h"
#include "Components/TextBlock.h"

void UDamageNumberWidget::SetDamage(float Damage)
{
	if (DamageText)
		DamageText->SetText(FText::AsNumber(FMath::RoundToInt(Damage)));
}
