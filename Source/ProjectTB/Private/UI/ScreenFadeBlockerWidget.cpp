#include "UI/ScreenFadeBlockerWidget.h"

void UScreenFadeBlockerWidget::PlayFadeOutAndRemove_Implementation()
{
	RemoveImmediately();
}

void UScreenFadeBlockerWidget::NotifyFadeOutFinished()
{
	RemoveImmediately();
}

void UScreenFadeBlockerWidget::RemoveImmediately()
{
	RemoveFromParent();
}
