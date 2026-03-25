#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ScreenFadeBlockerWidget.generated.h"

/**
 * 맵 전환 중 화면 가리게 (페이드 인 아웃으로 해결할 수 없는 위젯들 가림막)
 */
UCLASS(Blueprintable)
class PROJECTTB_API UScreenFadeBlockerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	//블루프린트로 위젯을 페이드아웃 하면서 제거
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Fade")
	void PlayFadeOutAndRemove();
	void PlayFadeOutAndRemove_Implementation();

	// 애니메이션 재생 끝남을 알림
	UFUNCTION(BlueprintCallable, Category="Fade")
	void NotifyFadeOutFinished();

	// 즉시 삭제
	UFUNCTION(BlueprintCallable, Category="Fade")
	void RemoveImmediately();
};
