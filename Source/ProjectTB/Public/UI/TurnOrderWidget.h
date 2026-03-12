#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TurnOrderWidget.generated.h"

class ABattleCombatant;

/**
 * 좌측 상단 턴 순서 패널.
 * BattleManager.OnTurnOrderUpdated 델리게이트를 구독해 자동 갱신.
 * 실제 레이아웃/Portrait 표시는 Blueprint에서 구현.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTTB_API UTurnOrderWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 턴 순서가 바뀔 때마다 호출됨
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="TurnOrder")
	void UpdateTurnOrder(const TArray<ABattleCombatant*>& UpcomingTurns);
	virtual void UpdateTurnOrder_Implementation(const TArray<ABattleCombatant*>& UpcomingTurns) {}
};
