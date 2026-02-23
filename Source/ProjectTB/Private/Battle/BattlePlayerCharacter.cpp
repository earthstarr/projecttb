#include "Battle/BattlePlayerCharacter.h"

ABattlePlayerCharacter::ABattlePlayerCharacter() {}

void ABattlePlayerCharacter::OnTurnBegin_Implementation()
{
	Super::OnTurnBegin_Implementation();
	// BattleManager가 BattleMenuWidget을 열어 입력 대기
}

void ABattlePlayerCharacter::OnTurnEnd_Implementation()
{
	Super::OnTurnEnd_Implementation();
}
