#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "TBGameplayTags.h"
#include "TBGameplayAbility.generated.h"

UENUM(BlueprintType)
enum class EAbilityCostType : uint8
{
	None,
	Stamina,
	MP,
	HP
};

UENUM(BlueprintType)
enum class EAbilityTargetType : uint8
{
	Self,
	SingleEnemy,
	AllEnemies,
	SingleAlly,
	AllAllies
};

UENUM(BlueprintType)
enum class EAbilityAnimType : uint8
{
	Melee,   // 캐릭터가 타겟에게 이동 후 공격, 복귀
	Ranged   // 제자리 캐스팅 + 타겟 위치에 이펙트 스폰
};

/**
 * 모든 배틀 어빌리티의 기반 클래스.
 * 개별 어빌리티는 Blueprint에서 이 클래스를 상속해 만든다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PROJECTTB_API UTBGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UTBGameplayAbility();

	// ─── UI 표시 정보 ────────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display")
	FText AbilityDisplayName;

	// false로 설정하면 어빌리티 메뉴에 표시되지 않음 (기본 공격 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display")
	bool bShowInAbilityMenu = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display")
	FText AbilityDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display")
	TObjectPtr<UTexture2D> AbilityIcon;

	// ─── 비용 (UI 표시용 — 실제 차감은 CostGameplayEffectClass로) ──────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cost")
	EAbilityCostType CostType = EAbilityCostType::Stamina;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cost")
	float CostAmount = 0.f;

	// ─── 타겟팅 ──────────────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Targeting")
	EAbilityTargetType TargetType = EAbilityTargetType::SingleEnemy;

	// ─── 애니메이션 ──────────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	EAbilityAnimType AnimationType = EAbilityAnimType::Melee;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UAnimMontage> AbilityMontage;

	// Melee 전용: 타겟까지 이동 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation", meta=(EditCondition="AnimationType==EAbilityAnimType::Melee"))
	float MoveSpeed = 600.f;

	// ─── 데미지 ──────────────────────────────────────────────────────────────
	// 설정하면 ActivateAbility를 Blueprint 없이 C++에서 자동 처리
	// 비워두면 Blueprint의 Event ActivateAbility가 실행됨
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// TBDamageExecution에 SetByCaller로 전달될 배율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage")
	float AbilityMultiplier = 1.0f;

	// Physical / Magic 태그 (TBDamageExecution에서 읽음, GE 스펙 소스 태그에 주입)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage")
	FGameplayTag AbilityTypeTag;

	// ─── 유틸 ────────────────────────────────────────────────────────────────
	// UI에서 비용 지불 가능 여부 확인 (그레이아웃 처리용)
	UFUNCTION(BlueprintCallable, Category="Ability")
	bool CanAffordCost(UAbilitySystemComponent* ASC) const;

protected:
	// GAS 비용 체크 override — CanAffordCost를 실제 활성화 전 검증에 연결
	virtual bool CheckCost(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	// 몽타주 완료 시 호출 (데미지 적용 + 어빌리티 종료)
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageBlendOut();

	// GE 적용 + BattleManager 완료 알림 + EndAbility
	void ApplyDamageAndFinish();

	bool bAbilityFinished = false;
	FVector OriginLocation = FVector::ZeroVector;

	FTimerHandle PreMoveTimer;
	FTimerHandle PreMontageTimer;

	UFUNCTION() void DoTeleportToTarget();
	UFUNCTION() void DoPlayMontage();
};
