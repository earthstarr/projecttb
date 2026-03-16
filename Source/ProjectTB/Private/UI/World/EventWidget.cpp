// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/World/EventWidget.h"
#include "World/LevelActor/Event/EventBase.h"
#include "TBGameInstance.h"

void UEventWidget::SetOwnerEvent(AEventBase* InOwnerEvent)
{
	OwnerEvent = InOwnerEvent;
}

void UEventWidget::RequestCloseEvent()
{
	if (OwnerEvent == nullptr)
	{
		return;
	}

	OwnerEvent->RequestCloseEvent();
}