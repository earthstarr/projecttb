#include "Battle/BattleCombatant.h"
#include "AbilitySystemComponent.h"
#include "Attributes/TBAttributeSet.h"
#include "Abilities/TBGameplayAbility.h"
#include "TBGameplayTags.h"
#include "Components/WidgetComponent.h"
#include "UI/DamageNumberWidget.h"

ABattleCombatant::ABattleCombatant()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet           = CreateDefaultSubobject<UTBAttributeSet>(TEXT("AttributeSet"));

	DamageWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("DamageWidget"));
	DamageWidgetComponent->SetupAttachment(GetRootComponent());
	DamageWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 100.f)); // 머리 위
	DamageWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);           // 항상 카메라 향함
	DamageWidgetComponent->SetVisibility(false);
}

UAbilitySystemComponent* ABattleCombatant::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABattleCombatant::BeginPlay()
{
	Super::BeginPlay();
	InitAbilitySystem();
}

void ABattleCombatant::InitAbilitySystem()
{
	if (bAbilitySystemInitialized || !AbilitySystemComponent) return;

	// GAS 활성화, 싱글플레이어: Owner = Avatar = this
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	ApplyStartingEffects();    // 초기 스탯 세팅 (HP=300, Speed=80 등)
	GrantStartingAbilities();  // 어빌리티 부여

	bAbilitySystemInitialized = true;
}

void ABattleCombatant::ApplyStartingEffects()
{
	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	for (const TSubclassOf<UGameplayEffect>& EffectClass : StartingEffects)
	{
		if (!EffectClass) continue;
		FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, Context);
		if (Spec.IsValid())
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void ABattleCombatant::GrantStartingAbilities()
{
	for (const TSubclassOf<UTBGameplayAbility>& AbilityClass : StartingAbilities)
	{
		if (AbilityClass)
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
	}
}

// ─── 어트리뷰트 접근자 ────────────────────────────────────────────────────────
float ABattleCombatant::GetHP()         const { return AttributeSet ? AttributeSet->GetHP()         : 0.f; }
float ABattleCombatant::GetMaxHP()      const { return AttributeSet ? AttributeSet->GetMaxHP()      : 0.f; }
float ABattleCombatant::GetMP()         const { return AttributeSet ? AttributeSet->GetMP()         : 0.f; }
float ABattleCombatant::GetMaxMP()      const { return AttributeSet ? AttributeSet->GetMaxMP()      : 0.f; }
float ABattleCombatant::GetStamina()    const { return AttributeSet ? AttributeSet->GetStamina()    : 0.f; }
float ABattleCombatant::GetMaxStamina() const { return AttributeSet ? AttributeSet->GetMaxStamina() : 0.f; }
float ABattleCombatant::GetSpeed()      const { return AttributeSet ? AttributeSet->GetSpeed()      : 0.f; }
bool  ABattleCombatant::IsDead()        const { return GetHP() <= 0.f; }

TArray<UTBGameplayAbility*> ABattleCombatant::GetGrantedAbilities() const
{
	TArray<UTBGameplayAbility*> Result;
	if (!AbilitySystemComponent) return Result;

	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		// 인스턴스가 있으면 인스턴스, 없으면 CDO 반환
		if (UTBGameplayAbility* Instance = Cast<UTBGameplayAbility>(Spec.GetPrimaryInstance()))
			Result.Add(Instance);
		else if (UTBGameplayAbility* CDO = Cast<UTBGameplayAbility>(Spec.Ability.Get()))
			Result.Add(CDO);
	}
	return Result;
}

// ─── AnimNotify 데미지 연결 ────────────────────────────────────────────────────
void ABattleCombatant::OnHitNotify(int32 HitIndex)
{
	if (!AbilitySystemComponent) return;

	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (UTBGameplayAbility* Instance = Cast<UTBGameplayAbility>(Spec.GetPrimaryInstance()))
		{
			if (Instance->IsActive())
			{
				Instance->ApplyDamage(HitIndex);
				return;
			}
		}
	}
}

void ABattleCombatant::OnSpawnImpactNotify(int32 HitIndex)
{
	if (!AbilitySystemComponent) return;

	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (UTBGameplayAbility* Instance = Cast<UTBGameplayAbility>(Spec.GetPrimaryInstance()))
		{
			if (Instance->IsActive())
			{
				// 어빌리티의 스폰 함수 호출
				Instance->RequestSpawnImpact(HitIndex);
				return;
			}
		}
	}
}

// ─── 사망 처리 (AttributeSet에서 호출) ────────────────────────────────────────
void ABattleCombatant::OnDeathInternal()
{
	AbilitySystemComponent->AddLooseGameplayTag(TAG_Combatant_State_Dead);
	AbilitySystemComponent->CancelAllAbilities();
	OnDeath.Broadcast(this);

	// 0.5초 후 Actor 제거
	FTimerHandle DestroyTimer;
	GetWorldTimerManager().SetTimer(DestroyTimer, this, &ABattleCombatant::DestroyAfterDeath, 0.5f, false);
}

void ABattleCombatant::DestroyAfterDeath()
{
	Destroy();
}

void ABattleCombatant::OnDamageReceivedInternal(float Damage)
{
	OnDamageReceived.Broadcast(this, Damage);
	ShowDamageNumber(Damage);
}

void ABattleCombatant::ShowDamageNumber(float Damage)
{
	if (!DamageWidgetComponent) return;

	// 위젯 클래스가 설정된 경우 최초 1회 세팅
	if (DamageNumberWidgetClass && !DamageWidgetComponent->GetWidget())
		DamageWidgetComponent->SetWidgetClass(DamageNumberWidgetClass);

	if (UDamageNumberWidget* Widget = Cast<UDamageNumberWidget>(DamageWidgetComponent->GetUserWidgetObject()))
		Widget->SetDamage(Damage);

	DamageWidgetComponent->SetVisibility(true);

	// 기존 타이머 초기화 후 2초 뒤 숨기기
	GetWorldTimerManager().ClearTimer(DamageHideTimer);
	GetWorldTimerManager().SetTimer(DamageHideTimer, this, &ABattleCombatant::HideDamageNumber, 2.0f, false);
}

void ABattleCombatant::HideDamageNumber()
{
	if (DamageWidgetComponent)
		DamageWidgetComponent->SetVisibility(false);
}
