// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EventWidget.generated.h"

class AEventBase;

class AEventBase;
/**
 * 
 */
UCLASS()
class PROJECTTB_API UEventWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="Event")
	void SetOwnerEvent(AEventBase* InOwnerEvent);

	UFUNCTION(BlueprintCallable, Category="Event")
	void RequestCloseEvent();

protected:
	UPROPERTY(BlueprintReadOnly, Category="Event")
	TObjectPtr<AEventBase> OwnerEvent;
	
	
	
};
