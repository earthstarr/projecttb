#include "TBGameInstance.h"
#include "AbilitySystemGlobals.h"

void UTBGameInstance::Init()
{
	Super::Init();

	// GAS 전역 초기화 — 반드시 한 번 호출해야 ExecutionCalculation 등이 작동함
	UAbilitySystemGlobals::Get().InitGlobalData();
}
