#include "Abilities/TBDamageExecution.h"
#include "Attributes/TBAttributeSet.h"
#include "TBGameplayTags.h"

// ─── 어트리뷰트 캡처 구조체 ────────────────────────────────────────────────
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
		// Source(공격자) 캡처
		DEFINE_ATTRIBUTE_CAPTUREDEF(UTBAttributeSet, PhysicalAttack,    Source, false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UTBAttributeSet, MagicAttack,       Source, false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UTBAttributeSet, CriticalChance,    Source, false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UTBAttributeSet, CriticalMultiplier,Source, false)
		// Target(피격자) 캡처
		DEFINE_ATTRIBUTE_CAPTUREDEF(UTBAttributeSet, PhysicalDefense,   Target, false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UTBAttributeSet, MagicDefense,      Target, false)
	}
};

static const FTBDamageCaptureStruct& GetCaptureStruct()
{
	static FTBDamageCaptureStruct CaptureStruct;
	return CaptureStruct;
}

// ─── 생성자: 캡처할 어트리뷰트 등록 ────────────────────────────────────────
UTBDamageExecution::UTBDamageExecution()
{
	RelevantAttributesToCapture.Add(GetCaptureStruct().PhysicalAttackDef);
	RelevantAttributesToCapture.Add(GetCaptureStruct().MagicAttackDef);
	RelevantAttributesToCapture.Add(GetCaptureStruct().PhysicalDefenseDef);
	RelevantAttributesToCapture.Add(GetCaptureStruct().MagicDefenseDef);
	RelevantAttributesToCapture.Add(GetCaptureStruct().CriticalChanceDef);
	RelevantAttributesToCapture.Add(GetCaptureStruct().CriticalMultiplierDef);
}

// ─── 실제 데미지 계산 ────────────────────────────────────────────────────────
void UTBDamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// 물리/마법 구분
	const bool bIsPhysical = Spec.CapturedSourceTags.GetAggregatedTags()->HasTag(TAG_Effect_Damage_Physical);

	// 어빌리티 배율 (SetByCaller)
	const float AbilityMultiplier = Spec.GetSetByCallerMagnitude(TAG_Data_AbilityMultiplier, false, 1.0f);

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

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

	// ─── 데미지 공식 ─────────────────────────────────────────────────────────
	// Reduction = Defense / (Defense + 100) → 0~1, 절대 1 불가
	const float Reduction  = Defense / (Defense + 100.f);
	const float BaseDamage = Attack * AbilityMultiplier;
	float FinalDamage      = BaseDamage * (1.f - Reduction);

	// ±10% 랜덤 편차
	FinalDamage *= FMath::RandRange(0.9f, 1.1f);

	// 크리티컬 판정
	if (FMath::RandRange(0.f, 1.f) < CritChance)
		FinalDamage *= CritMulti;

	// 최소 1 보장
	FinalDamage = FMath::Max(FinalDamage, 1.f);

	// IncomingDamage 메타 어트리뷰트에 출력
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		UTBAttributeSet::GetIncomingDamageAttribute(),
		EGameplayModOp::Additive,
		FinalDamage));
}
