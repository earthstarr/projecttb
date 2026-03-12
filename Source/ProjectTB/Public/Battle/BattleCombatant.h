#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "Data/LevelDataTypes.h"
#include "BattleCombatant.generated.h"

// ─── 런타임 상태이상 인스턴스 ────────────────────────────────────────────────
// 별도 시전자 추적 없이 스냅샷 Magnitude만 저장.
// 같은 Tag라도 시전자가 다르면 별도 인스턴스로 공존.
USTRUCT(BlueprintType)
struct FStatusEffectInstance
{
	GENERATED_BODY()

	// Status.Burn / Status.Poison / Status.Regen / Status.Stun
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag StatusTag;

	// 부여 시점에 계산된 틱당 기본 데미지/힐 (시전자 MagicAttack 스냅샷)
	// Stun은 0. Poison은 HP% 부분이 틱마다 실시간 추가됨.
	UPROPERTY(BlueprintReadOnly)
	float MagnitudePerStack = 0.f;

	// 남은 스택 수 = 남은 턴 수. 0 되면 제거.
	UPROPERTY(BlueprintReadOnly)
	int32 RemainingTurns = 0;
};

class UAbilitySystemComponent;
class UTBAttributeSet;
class UTBGameplayAbility;
class UGameplayEffect;
class UWidgetComponent;
class UDamageNumberWidget;
class UCanvasPanel;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatantDeath,       ABattleCombatant*, Combatant);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDamageReceived,     ABattleCombatant*, Combatant, float, Damage, bool, bIsCritical);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealReceived,        ABattleCombatant*, Combatant, float, Heal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatChanged,          ABattleCombatant*, Combatant);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStatusEffectsChanged, ABattleCombatant*, Combatant);

/**
 * 모든 전투 유닛(플레이어/적)의 기반 클래스.
 * ASC와 AttributeSet을 직접 보유한다 (PlayerState 아님).
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PROJECTTB_API ABattleCombatant : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABattleCombatant();

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// ─── 캐릭터 정보 (UI 표시용) ─────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity")
	FText CharacterName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Identity")
	TObjectPtr<UTexture2D> Portrait;

	// ─── GAS 초기 설정 (Blueprint에서 세팅) ──────────────────────────────────
	// 어빌리티: BeginPlay에서 ASC에 부여됨
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GAS")
	TArray<TSubclassOf<UTBGameplayAbility>> StartingAbilities;

	// 초기 스탯 GE (Instant): 캐릭터마다 다른 스탯 세팅
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GAS")
	TArray<TSubclassOf<UGameplayEffect>> StartingEffects;

	// ─── 어트리뷰트 접근자 ───────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category="Attributes") float GetHP() const;
	UFUNCTION(BlueprintCallable, Category="Attributes") float GetMaxHP() const;
	UFUNCTION(BlueprintCallable, Category="Attributes") float GetMP() const;
	UFUNCTION(BlueprintCallable, Category="Attributes") float GetMaxMP() const;
	UFUNCTION(BlueprintCallable, Category="Attributes") float GetStamina() const;
	UFUNCTION(BlueprintCallable, Category="Attributes") float GetMaxStamina() const;
	UFUNCTION(BlueprintCallable, Category="Attributes") float GetMagicAttack() const;
	// 독 상태이상 시 Speed 30% 감소 반영
	UFUNCTION(BlueprintCallable, Category="Attributes") float GetSpeed() const;
	UFUNCTION(BlueprintCallable, Category="Attributes") bool  IsDead() const;

	// 현재 부여된 어빌리티 목록 (BattleMenuWidget에서 사용)
	UFUNCTION(BlueprintCallable, Category="Abilities")
	TArray<UTBGameplayAbility*> GetGrantedAbilities() const;

	// ─── 이벤트 델리게이트 ───────────────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnCombatantDeath OnDeath;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnDamageReceived OnDamageReceived;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnHealReceived OnHealReceived;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnStatChanged OnStatChanged;

	// 상태이상 추가/제거/틱 시 브로드캐스트 (UI 아이콘 갱신용)
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnStatusEffectsChanged OnStatusEffectsChanged;

	// AttributeSet에서 직접 호출 (내부용)
	void OnDeathInternal();
	void OnDamageReceivedInternal(float Damage, bool bIsCritical = false);
	void OnHealReceivedInternal(float Heal);
	void OnStatChangedInternal();

	// ─── 데미지 숫자 위젯 ────────────────────────────────────────────────────
	// Blueprint에서 WBP_DamageNumber 클래스를 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UUserWidget> DamageNumberWidgetClass;

	// ─── 타겟 인디케이터 (머리 위 화살표) ────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UUserWidget> TargetIndicatorWidgetClass;

	// ─── 상태이상 아이콘 패널 (머리 위) ─────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UUserWidget> StatusIconWidgetClass;

	UFUNCTION(BlueprintCallable, Category="UI")
	void ShowTargetIndicator();

	UFUNCTION(BlueprintCallable, Category="UI")
	void HideTargetIndicator();

	// AnimNotify_OnHit → 현재 활성 어빌리티의 ApplyDamage 호출
	// HitIndex: TBGameplayAbility::HitMultipliers 배열 인덱스
	UFUNCTION(BlueprintCallable, Category="Battle")
	void OnHitNotify(int32 HitIndex);

	// ranged일때 스폰 노티파이
	UFUNCTION(BlueprintCallable, Category="Battle")
	void OnSpawnImpactNotify(int32 HitIndex = 0);

	// AnimNotify_OpenParryTiming → BattleManager의 패링 타이밍 열기
	UFUNCTION(BlueprintCallable, Category="Battle")
	void OnOpenParryTimingNotify(float Duration = 0.2f);

	// 패링 성공 시 BattleManager에서 호출
	void PlayParryMontage();

	// 패링 몽타주 (Blueprint에서 설정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UAnimMontage> ParryMontage;

	// 패링 성공 이펙트 (Blueprint에서 설정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UNiagaraSystem> ParryEffect;

	// 패링 이펙트 스폰 위치 오프셋 (캐릭터 기준)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	FVector ParryEffectOffset = FVector(50.f, 0.f, 80.f);

	// ─── 상태이상 ──────────────────────────────────────────────────────────────
	// 새 상태이상 인스턴스 추가 (어빌리티에서 호출)
	UFUNCTION(BlueprintCallable, Category="Battle|StatusEffect")
	void ApplyStatusEffect(const FStatusEffectInstance& NewEffect);

	// 턴 시작 시 BattleManager가 호출. 반환값: 스턴 여부 (true = 턴 스킵)
	bool TickStatusEffects();

	// 현재 활성 상태이상 목록 (UI 참조용)
	UPROPERTY(BlueprintReadOnly, Category="Battle|StatusEffect")
	TArray<FStatusEffectInstance> ActiveStatusEffects;

	// ─── 방어 상태 ────────────────────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category="Battle")
	void SetDefending(bool bDefending);

	UFUNCTION(BlueprintCallable, Category="Battle")
	bool IsDefending() const;

	// ─── 턴 알림 (Blueprint에서 오버라이드 가능) ──────────────────────────────
	UFUNCTION(BlueprintNativeEvent, Category="Battle")
	void OnTurnBegin();
	virtual void OnTurnBegin_Implementation() {}

	UFUNCTION(BlueprintNativeEvent, Category="Battle")
	void OnTurnEnd();
	virtual void OnTurnEnd_Implementation() {}

	// 어빌리티 시작 전 회전 저장 (FinishAbility에서 복귀용)
	UPROPERTY(BlueprintReadOnly, Category="Battle")
	FRotator PreAbilityRotation;

	// ─── ATB 게이지 시스템 ────────────────────────────────────────────────────
	// 행동 게이지 (0~100, 100 이상이면 행동 가능)
	UPROPERTY(BlueprintReadOnly, Category="Battle|ATB")
	float ActionGauge = 0.f;

	// 행동에 필요한 게이지 (기본 100)
	static constexpr float ActionGaugeThreshold = 100.f;

	// 게이지 충전 (Speed만큼 증가)
	void ChargeActionGauge();

	// 행동 후 게이지 소모 (Threshold만큼 차감)
	void ConsumeActionGauge();

	// 게이지가 Threshold 이상인지
	UFUNCTION(BlueprintCallable, Category="Battle|ATB")
	bool IsActionReady() const { return ActionGauge >= ActionGaugeThreshold; }

	// ─── 레벨 스탯 적용 ─────────────────────────────────────────────────────────
	/** DataTable + GameInstance 기반으로 스탯 적용 (플레이어 캐릭터용) */
	UFUNCTION(BlueprintCallable, Category="Stats")
	void ApplyLevelStats(const FCharacterLevelStats& LevelStats, const FPartyMemberData& PartyData);

	/** 스탯을 직접 적용 (FCharacterLevelStats 기반, 적 캐릭터용) */
	UFUNCTION(BlueprintCallable, Category="Stats")
	void ApplyStatsDirectly(const FCharacterLevelStats& Stats);

protected:
	virtual void BeginPlay() override;

	void InitAbilitySystem();
	void GrantStartingAbilities();
	void ApplyStartingEffects();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="UI")
	TObjectPtr<UWidgetComponent> DamageWidgetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="UI")
	TObjectPtr<UWidgetComponent> TargetIndicatorComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="UI")
	TObjectPtr<UWidgetComponent> StatusIconComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
	TObjectPtr<UTBAttributeSet> AttributeSet;

private:
	bool bAbilitySystemInitialized = false;
	void DestroyAfterDeath();
	void HandleGameplayEffectApplied(UAbilitySystemComponent* Target, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle Handle);

	// 다중 데미지 숫자 스택 시스템
	void SpawnDamageNumber(float Value, bool bIsHeal, bool bIsCritical = false);
	void OnDamageNumberRemoved(UDamageNumberWidget* Widget);

	// 현재 표시 중인 데미지 위젯 목록
	UPROPERTY()
	TArray<TObjectPtr<UDamageNumberWidget>> ActiveDamageNumbers;

	// 데미지 숫자 스택 간격 (픽셀)
	float DamageNumberStackSpacing = 30.f;

	// 상태이상 틱: 동적 GE로 IncomingDamage/IncomingHeal 메타 속성에 주입
	void ApplyStatusTickDamage(float Damage);
	void ApplyStatusTickHeal(float Heal);

	// ActiveStatusEffects → GAS Loose Tag 동기화
	void SyncStatusTags();
};
