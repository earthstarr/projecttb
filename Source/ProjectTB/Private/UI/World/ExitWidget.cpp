// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/World/ExitWidget.h"

#include "World/WorldPlayerController.h"

void UExitWidget::RequestCloseWidget()
{
	//플레이어 컨트롤러 말고 플레이어 허드를 찾아가도록 수정해줄것 todo

	if (OwnerWidget == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UExitWidget클래스 RequestCloseWidget: OwnerWidget is nullptr"));
		return;
	}
	else
	{
		AWorldPlayerController* PC = Cast<AWorldPlayerController>(GetOwningPlayer());
		if (PC != nullptr)
		{
			PC->CloseWidget(OwnerWidget, true);
		}
	}
}
