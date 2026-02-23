#include "UI/EnemyHealthBarWidget.h"
#include "Components/ProgressBar.h"

void UEnemyHealthBarWidget::UpdateHealth(float CurrentHP, float MaxHP)
{
	if (HealthBar && MaxHP > 0.f)
		HealthBar->SetPercent(CurrentHP / MaxHP);
}
