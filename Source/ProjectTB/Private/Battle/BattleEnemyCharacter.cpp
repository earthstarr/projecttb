#include "Battle/BattleEnemyCharacter.h"
#include "Abilities/TBGameplayAbility.h"
#include "Components/WidgetComponent.h"
#include "UI/EnemyHealthBarWidget.h"

ABattleEnemyCharacter::ABattleEnemyCharacter()
{
	HealthBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidget"));
	HealthBarWidgetComponent->SetupAttachment(GetRootComponent());
	HealthBarWidgetComponent->SetRelativeLocation(FVector(0.f, 0.f, 120.f)); // 데미지 숫자보다 아래
	HealthBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidgetComponent->SetDrawSize(FVector2D(150.f, 15.f));
}

void ABattleEnemyCharacter::BeginPlay()
{
	Super::BeginPlay(); // BattleCombatant::BeginPlay → InitAbilitySystem (스탯 세팅 완료)

	if (HealthBarWidgetClass)
		HealthBarWidgetComponent->SetWidgetClass(HealthBarWidgetClass);

	// 초기 체력바 표시
	RefreshHealthBar();

	// 데미지 받을 때마다 체력바 갱신
	OnDamageReceived.AddDynamic(this, &ABattleEnemyCharacter::OnDamageReceivedHandler);
}

void ABattleEnemyCharacter::OnDamageReceivedHandler(ABattleCombatant* /*Combatant*/, float /*Damage*/)
{
	RefreshHealthBar();
}

void ABattleEnemyCharacter::RefreshHealthBar()
{
	if (UEnemyHealthBarWidget* Widget = Cast<UEnemyHealthBarWidget>(HealthBarWidgetComponent->GetUserWidgetObject()))
		Widget->UpdateHealth(GetHP(), GetMaxHP());
}

void ABattleEnemyCharacter::SelectAction_Implementation(
	const TArray<ABattleCombatant*>& PlayerParty,
	ABattleCombatant*& OutTarget,
	TSubclassOf<UTBGameplayAbility>& OutAbilityClass)
{
	// 기본 AI: 살아있는 플레이어 중 랜덤 타겟 + 첫 번째 어빌리티
	TArray<ABattleCombatant*> Living = PlayerParty.FilterByPredicate(
		[](const ABattleCombatant* C) { return C && !C->IsDead(); });

	if (Living.IsEmpty()) return;

	OutTarget = Living[FMath::RandRange(0, Living.Num() - 1)];

	TArray<UTBGameplayAbility*> Abilities = GetGrantedAbilities();
	if (!Abilities.IsEmpty())
		OutAbilityClass = Abilities[0]->GetClass();
}

void ABattleEnemyCharacter::OnTurnBegin_Implementation()
{
	Super::OnTurnBegin_Implementation();
	// BattleManager가 SelectAction 호출 후 자동 실행
}
