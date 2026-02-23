#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "BattleCombatant.generated.h"

class UAbilitySystemComponent;
class UTBAttributeSet;
class UTBGameplayAbility;
class UGameplayEffect;
class USpringArmComponent;
class UCameraComponent;
class UWidgetComponent;
class UDamageNumberWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatantDeath,    ABattleCombatant*, Combatant);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDamageReceived,   ABattleCombatant*, Combatant, float, Damage);

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

	// AttributeSet에서 직접 호출 (내부용)
	void OnDeathInternal();
	void OnDamageReceivedInternal(float Damage);

	// ─── 데미지 숫자 위젯 ────────────────────────────────────────────────────
	// Blueprint에서 WBP_DamageNumber 클래스를 지정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UUserWidget> DamageNumberWidgetClass;

	// ─── 턴 알림 (Blueprint에서 오버라이드 가능) ──────────────────────────────
	UFUNCTION(BlueprintNativeEvent, Category="Battle")
	void OnTurnBegin();
	virtual void OnTurnBegin_Implementation() {}

	UFUNCTION(BlueprintNativeEvent, Category="Battle")
	void OnTurnEnd();
	virtual void OnTurnEnd_Implementation() {}

protected:
	virtual void BeginPlay() override;

	void InitAbilitySystem();
	void GrantStartingAbilities();
	void ApplyStartingEffects();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="UI")
	TObjectPtr<UWidgetComponent> DamageWidgetComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	TObjectPtr<USpringArmComponent> CameraSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	TObjectPtr<UCameraComponent> BattleCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
	TObjectPtr<UTBAttributeSet> AttributeSet;

private:
	bool bAbilitySystemInitialized = false;
	void DestroyAfterDeath();

	void ShowDamageNumber(float Damage);
	void HideDamageNumber();
	FTimerHandle DamageHideTimer;
};
