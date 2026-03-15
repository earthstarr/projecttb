// Fill out your copyright notice in the Description page of Project Settings.


#include "World/LevelActor/Event/EventBase.h"

#include "Blueprint/UserWidget.h"
#include "Components/PrimitiveComponent.h"
#include "UI/World/EventWidget.h"
#include "World/WorldCharacterBase.h"
#include "World/WorldPlayerController.h"

// Sets default values
AEventBase::AEventBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AEventBase::BeginPlay()
{
	Super::BeginPlay();
	
}

bool AEventBase::CanStartEvent() const
{
	if (bIsCompleted)
	{
		return false;
	}

	if (bIsTriggered)
	{
		return false;
	}

	return true;
}


// Called every frame
void AEventBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEventBase::OnAreaOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!CanStartEvent())
	{
		return;
	}
	
	AWorldCharacterBase* Player = Cast<AWorldCharacterBase>(OtherActor);
	if (Player == nullptr)
	{
		return;
	}

	AWorldPlayerController* PC = Cast<AWorldPlayerController>(Player->GetController());
	if (PC == nullptr)
	{
		return;
	}

	CachedPC = PC;
	bIsTriggered = true;
	bIsResolved = false;

	RequestShowEventWidget(PC);
	StartEvent();
}


bool AEventBase::CreateEventWidget()
{
	if (EventWidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AEventBase::CreateEventWidget - EventWidgetClass is nullptr"));
		return false;
	}

	EventWidget = CreateWidget<UUserWidget>(GetWorld(), EventWidgetClass);
	return EventWidget != nullptr;
}


void AEventBase::RequestShowEventWidget(AWorldPlayerController* PC)
{
	if (EventWidget == nullptr)
	{
		if (CreateEventWidget() == false)
		{
			return;
		}
	}
	
	if (UEventWidget* TypedWidget = Cast<UEventWidget>(EventWidget))
	{
		TypedWidget->SetOwnerEvent(this);
	}
	
	PC->ShowWidget(EventWidget, true);
}

void AEventBase::ResolveEvent()
{
	if (bIsTriggered == false)
	{
		return;
	}

	if (bIsCompleted == true)
	{
		return;
	}
	bIsResolved = true;
}

void AEventBase::RequestCloseEvent()
{
	if (!bIsResolved)
	{
		return;
	}

	FinishEvent();
}

void AEventBase::FinishEvent()
{
	if (bIsCompleted)
	{
		return;
	}

	bIsCompleted = true;
	bIsTriggered = false;

	if (CachedPC && EventWidget)
	{
		CachedPC->CloseWidget(EventWidget, true);
	}

	OnEventFinished();
	DisableEventActor();
}

void AEventBase::DisableEventActor()
{
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);

	if (!bOneShot)
	{
		return;
	}

	SetActorEnableCollision(false);
}

