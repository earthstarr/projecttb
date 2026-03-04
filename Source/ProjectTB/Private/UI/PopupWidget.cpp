// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/PopupWidget.h"

FReply UPopupWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 위젯 영역 밖 클릭이면 닫기
	if (!InGeometry.IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
	{
		RemoveFromParent();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UPopupWidget::SetPopupText(FString KeyText)
{
	//텍스트 로우 핸들에서 PopupText 찾기
	
}
