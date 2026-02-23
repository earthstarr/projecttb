#include "Attributes/TBAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Battle/BattleCombatant.h"

UTBAttributeSet::UTBAttributeSet()
{
	// 기본값 — StartingEffects(Blueprint GE)로 덮어씌워짐
	InitHP(100.f);
	InitMaxHP(100.f);
	InitMP(50.f);
	InitMaxMP(50.f);
	InitStamina(100.f);
	InitMaxStamina(100.f);
	InitSpeed(50.f);
	InitPhysicalAttack(10.f);
	InitMagicAttack(10.f);
	InitPhysicalDefense(5.f);
	InitMagicDefense(5.f);
	InitCriticalChance(0.05f);
	InitCriticalMultiplier(1.5f);
	InitIncomingDamage(0.f);
	InitIncomingHeal(0.f);
}

void UTBAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// 현재 값 범위 클램핑
	if      (Attribute == GetHPAttribute())               NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHP());
	else if (Attribute == GetMaxHPAttribute())            NewValue = FMath::Max(NewValue, 1.f);
	else if (Attribute == GetMPAttribute())               NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMP());
	else if (Attribute == GetMaxMPAttribute())            NewValue = FMath::Max(NewValue, 0.f);
	else if (Attribute == GetStaminaAttribute())          NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	else if (Attribute == GetMaxStaminaAttribute())       NewValue = FMath::Max(NewValue, 0.f);
	else if (Attribute == GetSpeedAttribute())            NewValue = FMath::Max(NewValue, 1.f);
	else if (Attribute == GetCriticalChanceAttribute())   NewValue = FMath::Clamp(NewValue, 0.f, 1.f);
	else if (Attribute == GetCriticalMultiplierAttribute()) NewValue = FMath::Max(NewValue, 1.f);
}

void UTBAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// ─── IncomingDamage → HP 차감 ───────────────────────────────────────────
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float DamageValue = GetIncomingDamage();
		SetIncomingDamage(0.f); // 메타 어트리뷰트 초기화

		if (DamageValue > 0.f)
		{
			const float NewHP = FMath::Max(GetHP() - DamageValue, 0.f);
			UE_LOG(LogTemp, Display, TEXT("TBAttributeSet: Damage=%.1f, HP %.1f → %.1f"), DamageValue, GetHP(), NewHP);
			SetHP(NewHP);

			if (ABattleCombatant* Combatant = Cast<ABattleCombatant>(GetOwningActor()))
			{
				Combatant->OnDamageReceivedInternal(DamageValue);
				if (NewHP <= 0.f)
					Combatant->OnDeathInternal();
			}
		}
	}
	// ─── IncomingHeal → HP 회복 ─────────────────────────────────────────────
	else if (Data.EvaluatedData.Attribute == GetIncomingHealAttribute())
	{
		const float HealValue = GetIncomingHeal();
		SetIncomingHeal(0.f);

		if (HealValue > 0.f)
			SetHP(FMath::Min(GetHP() + HealValue, GetMaxHP()));
	}
}
