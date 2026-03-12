#include "Abilities/TBDamageExecution.h"
#include "Attributes/TBAttributeSet.h"
#include "TBGameplayTags.h"

// ?�?�?� ?�트리뷰??캡처 구조�??�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�
struct FTBDamageCaptureStruct
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalAttack)
	DECLARE_ATTRIBUTE_CAPTUREDEF(MagicAttack)
	DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalDefense)
	DECLARE_ATTRIBUTE_CAPTUREDEF(MagicDefense)
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalChance)
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalMultiplier)

	FTBDamageCaptureStruct()
	{
		// Source(공격?? 캡처
		DEFINE_ATTRIBUTE_CAPTUREDEF(UTBAttributeSet, PhysicalAttack,    Source, false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UTBAttributeSet, MagicAttack,       Source, false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UTBAttributeSet, CriticalChance,    Source, false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UTBAttributeSet, CriticalMultiplier,Source, false)
		// Target(?�격?? 캡처
		DEFINE_ATTRIBUTE_CAPTUREDEF(UTBAttributeSet, PhysicalDefense,   Target, false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UTBAttributeSet, MagicDefense,      Target, false)
	}
};

static const FTBDamageCaptureStruct& GetCaptureStruct()
{
	static FTBDamageCaptureStruct CaptureStruct;
	return CaptureStruct;
}

// ?�?�?� ?�성?? 캡처???�트리뷰???�록 ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�
UTBDamageExecution::UTBDamageExecution()
{
	RelevantAttributesToCapture.Add(GetCaptureStruct().PhysicalAttackDef);
	RelevantAttributesToCapture.Add(GetCaptureStruct().MagicAttackDef);
	RelevantAttributesToCapture.Add(GetCaptureStruct().PhysicalDefenseDef);
	RelevantAttributesToCapture.Add(GetCaptureStruct().MagicDefenseDef);
	RelevantAttributesToCapture.Add(GetCaptureStruct().CriticalChanceDef);
	RelevantAttributesToCapture.Add(GetCaptureStruct().CriticalMultiplierDef);
}

// ?�?�?� ?�제 ?��?지/??계산 ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�
void UTBDamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// ???��? 체크
	const bool bIsHeal = Spec.CapturedSourceTags.GetAggregatedTags()->HasTag(TAG_Effect_Heal);

	// ?�빌리티 배율 (SetByCaller)
	const float AbilityMultiplier = Spec.GetSetByCallerMagnitude(TAG_Data_AbilityMultiplier, false, 1.0f);

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	// ?�?�?� ??처리 ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�
	if (bIsHeal)
	{
		float MagicAttack = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetCaptureStruct().MagicAttackDef, EvalParams, MagicAttack);

		float FinalHeal = MagicAttack * AbilityMultiplier;

		// ±10% ?�덤 ?�차
		FinalHeal *= FMath::RandRange(0.9f, 1.1f);

		// 최소 1 보장
		FinalHeal = FMath::Max(FinalHeal, 1.f);

		// IncomingHeal 메�? ?�트리뷰?�에 출력
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UTBAttributeSet::GetIncomingHealAttribute(),
			EGameplayModOp::Additive,
			FinalHeal));

		return;
	}

	// ?�?�?� ?��?지 처리 ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�
	// 물리/마법 구분
	const bool bIsPhysical = Spec.CapturedSourceTags.GetAggregatedTags()->HasTag(TAG_Effect_Damage_Physical);

	float Attack = 0.f, Defense = 0.f, CritChance = 0.f, CritMulti = 1.5f;

	if (bIsPhysical)
	{
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetCaptureStruct().PhysicalAttackDef,   EvalParams, Attack);
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetCaptureStruct().PhysicalDefenseDef,  EvalParams, Defense);
	}
	else
	{
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetCaptureStruct().MagicAttackDef,      EvalParams, Attack);
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetCaptureStruct().MagicDefenseDef,     EvalParams, Defense);
	}

	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetCaptureStruct().CriticalChanceDef,      EvalParams, CritChance);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetCaptureStruct().CriticalMultiplierDef,  EvalParams, CritMulti);

	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();

	// ─── 데미지 공식 ?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�?�
	// Reduction = Defense / (Defense + 100) ??0~1, ?��? 1 불�?
	const float Reduction  = Defense / (Defense + 100.f);	// 방어??50?�면 3분의2, 방어??100?�면 ?�반 ?��?지, 방어??200?�면 3분의1 ?��?지
	const float BaseDamage = Attack * AbilityMultiplier;
	float FinalDamage      = BaseDamage * (1.f - Reduction);

	// ±10% ?�덤 ?�차
	FinalDamage *= FMath::RandRange(0.9f, 1.1f);

	// ?�리?�컬 ?�정
	bool bIsCritical = false;
	const float CritRoll = FMath::RandRange(0.f, 1.f);
	if (CritRoll < CritChance)
	{
		FinalDamage *= CritMulti;
		bIsCritical = true;
	}

	// 방어 중인 ?��????��?지 50% 감소
	if (TargetASC && TargetASC->HasMatchingGameplayTag(TAG_Combatant_State_Defending))
		FinalDamage *= 0.5f;

	// ?�링 ?�공 ???��?지 50% 감소
	if (TargetASC && TargetASC->HasMatchingGameplayTag(TAG_Combatant_State_ParrySuccess))
		FinalDamage *= 0.5f;

	// 최소 1 보장
	FinalDamage = FMath::Max(FinalDamage, 1.f);

	// IncomingDamage 메�? ?�트리뷰?�에 출력
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		UTBAttributeSet::GetIncomingDamageAttribute(),
		EGameplayModOp::Additive,
		FinalDamage));

	// IncomingCritical 메�? ?�트리뷰?�에 ?�리?�컬 ?��? 출력
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		UTBAttributeSet::GetIncomingCriticalAttribute(),
		EGameplayModOp::Override,
		bIsCritical ? 1.f : 0.f));
}

