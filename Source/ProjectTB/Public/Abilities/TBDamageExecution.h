#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "TBDamageExecution.generated.h"

/**
 * 데미지 계산 실행 클래스.
 * Blueprint GE에서 ExecutionClass로 지정한다.
 *
 * 공식:
 *   Reduction   = Defense / (Defense + 100)
 *   BaseDamage  = Attack * AbilityMultiplier (SetByCaller)
 *   FinalDamage = BaseDamage * (1 - Reduction) * Variance(0.9~1.1)
 *   크리티컬 시: FinalDamage * CriticalMultiplier
 *
 * 태그로 물리/마법 구분:
 *   TAG_Effect_Damage_Physical → PhysicalAttack / PhysicalDefense
 *   TAG_Effect_Damage_Magic    → MagicAttack / MagicDefense
 */
UCLASS()
class PROJECTTB_API UTBDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UTBDamageExecution();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
