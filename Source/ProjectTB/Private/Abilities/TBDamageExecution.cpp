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

// ─── 실제 데미지/힐 계산 ────────────────────────────────────────────────────────
void UTBDamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// 힐 여부 체크
	const bool bIsHeal = Spec.CapturedSourceTags.GetAggregatedTags()->HasTag(TAG_Effect_Heal);

	// 어빌리티 배율 (SetByCaller)
	const float AbilityMultiplier = Spec.GetSetByCallerMagnitude(TAG_Data_AbilityMultiplier, false, 1.0f);

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	// ─── 힐 처리 ─────────────────────────────────────────────────────────────
	if (bIsHeal)
	{
		float MagicAttack = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(GetCaptureStruct().MagicAttackDef, EvalParams, MagicAttack);

		float FinalHeal = MagicAttack * AbilityMultiplier;

		// ±10% 랜덤 편차
		FinalHeal *= FMath::RandRange(0.9f, 1.1f);

		// 최소 1 보장
		FinalHeal = FMath::Max(FinalHeal, 1.f);

		UE_LOG(LogTemp, Warning, TEXT("TBDamageExecution: HEAL MagicAttack=%.1f, Multiplier=%.2f, FinalHeal=%.1f"),
			MagicAttack, AbilityMultiplier, FinalHeal);

		// IncomingHeal 메타 어트리뷰트에 출력
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UTBAttributeSet::GetIncomingHealAttribute(),
			EGameplayModOp::Additive,
			FinalHeal));

		return;
	}

	// ─── 데미지 처리 ─────────────────────────────────────────────────────────
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

	UE_LOG(LogTemp, Warning, TEXT("TBDamageExecution: Attack=%.1f, Defense=%.1f, CritChance=%.2f, CritMulti=%.2f"),
		Attack, Defense, CritChance, CritMulti);

	// ─── 데미지 공식 ─────────────────────────────────────────────────────────
	// Reduction = Defense / (Defense + 100) → 0~1, 절대 1 불가
	const float Reduction  = Defense / (Defense + 100.f);	// 방어력 50이면 3분의2, 방어력 100이면 절반 데미지, 방어력 200이면 3분의1 데미지
	const float BaseDamage = Attack * AbilityMultiplier;
	float FinalDamage      = BaseDamage * (1.f - Reduction);

	// ±10% 랜덤 편차
	FinalDamage *= FMath::RandRange(0.9f, 1.1f);

	// 크리티컬 판정
	bool bIsCritical = false;
	const float CritRoll = FMath::RandRange(0.f, 1.f);
	if (CritRoll < CritChance)
	{
		FinalDamage *= CritMulti;
		bIsCritical = true;
		UE_LOG(LogTemp, Warning, TEXT("TBDamageExecution: CRITICAL HIT! Roll=%.2f < Chance=%.2f, Damage=%.1f (x%.1f)"),
			CritRoll, CritChance, FinalDamage, CritMulti);
	}

	UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();

	// 방어 중인 타겟 → 데미지 50% 감소
	if (TargetASC && TargetASC->HasMatchingGameplayTag(TAG_Combatant_State_Defending))
		FinalDamage *= 0.5f;

	// 패링 성공 → 데미지 50% 감소
	if (TargetASC && TargetASC->HasMatchingGameplayTag(TAG_Combatant_State_ParrySuccess))
		FinalDamage *= 0.5f;

	// 최소 1 보장
	FinalDamage = FMath::Max(FinalDamage, 1.f);

	// IncomingDamage 메타 어트리뷰트에 출력
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		UTBAttributeSet::GetIncomingDamageAttribute(),
		EGameplayModOp::Additive,
		FinalDamage));

	// IncomingCritical 메타 어트리뷰트에 크리티컬 여부 출력
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		UTBAttributeSet::GetIncomingCriticalAttribute(),
		EGameplayModOp::Override,
		bIsCritical ? 1.f : 0.f));
}
