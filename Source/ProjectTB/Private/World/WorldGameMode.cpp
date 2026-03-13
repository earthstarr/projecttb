#include "World/WorldGameMode.h"
#include "World/WorldCharacter.h"

AWorldGameMode::AWorldGameMode()
{
	// DefaultPawnClass는 BP_WorldCharacter Blueprint에서 오버라이드해서 사용.
	// 여기서는 C++ 기본 클래스를 지정해 두기만 함.
	DefaultPawnClass = AWorldCharacter::StaticClass();
}
