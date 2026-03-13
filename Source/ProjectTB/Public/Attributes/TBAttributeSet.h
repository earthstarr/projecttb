#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "TBAttributeSet.generated.h"

// 어트리뷰트 접근자 매크로 (Getter/Setter/Initter 자동 생성)
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)           \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)               \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)               \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PROJECTTB_API UTBAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UTBAttributeSet();

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// ─── Health ─────────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category="Attributes|Health")
	FGameplayAttributeData HP;
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, HP)

	UPROPERTY(BlueprintReadOnly, Category="Attributes|Health")
	FGameplayAttributeData MaxHP;
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, MaxHP)

	// ─── Mana ────────────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category="Attributes|Mana")
	FGameplayAttributeData MP;
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, MP)

	UPROPERTY(BlueprintReadOnly, Category="Attributes|Mana")
	FGameplayAttributeData MaxMP;
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, MaxMP)

	// ─── Stamina ─────────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category="Attributes|Stamina")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, Category="Attributes|Stamina")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, MaxStamina)

	// ─── Combat Stats ────────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category="Attributes|Combat")
	FGameplayAttributeData Speed;
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, Speed)

	UPROPERTY(BlueprintReadOnly, Category="Attributes|Combat")
	FGameplayAttributeData PhysicalAttack;
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, PhysicalAttack)

	UPROPERTY(BlueprintReadOnly, Category="Attributes|Combat")
	FGameplayAttributeData MagicAttack;
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, MagicAttack)

	UPROPERTY(BlueprintReadOnly, Category="Attributes|Combat")
	FGameplayAttributeData PhysicalDefense;
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, PhysicalDefense)

	UPROPERTY(BlueprintReadOnly, Category="Attributes|Combat")
	FGameplayAttributeData MagicDefense;
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, MagicDefense)

	UPROPERTY(BlueprintReadOnly, Category="Attributes|Combat")
	FGameplayAttributeData CriticalChance;      // 0.0 ~ 1.0
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, CriticalChance)

	UPROPERTY(BlueprintReadOnly, Category="Attributes|Combat")
	FGameplayAttributeData CriticalMultiplier;  // 기본 1.5
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, CriticalMultiplier)

	// ─── Meta (계산 전용, 저장되지 않음) ────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category="Attributes|Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, IncomingDamage)

	UPROPERTY(BlueprintReadOnly, Category="Attributes|Meta")
	FGameplayAttributeData IncomingHeal;
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, IncomingHeal)

	// 크리티컬 히트 여부 (0 = 일반, 1 = 크리티컬)
	UPROPERTY(BlueprintReadOnly, Category="Attributes|Meta")
	FGameplayAttributeData IncomingCritical;
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, IncomingCritical)
	
	// ─── Dice Modifier ──────────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category="Attributes|Dice")
	FGameplayAttributeData DiceFaceBonus;
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, DiceFaceBonus)
	
	UPROPERTY(BlueprintReadOnly, Category="Attributes|Dice")
	FGameplayAttributeData DiceMinFace;
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, DiceMinFace)
	
	UPROPERTY(BlueprintReadOnly, Category="Attributes|Dice")
	FGameplayAttributeData DiceMaxFace;
	ATTRIBUTE_ACCESSORS(UTBAttributeSet, DiceMaxFace)
};
