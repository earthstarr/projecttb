// Fill out your copyright notice in the Description page of Project Settings.


#include "World/LevelActor/Event/EventBase.h"

#include "Blueprint/UserWidget.h"
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

// Called every frame
void AEventBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEventBase::OnAreaOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AWorldCharacterBase* Player = Cast<AWorldCharacterBase>(OtherActor);
	if (Player)
	{
		AWorldPlayerController* PC = Cast<AWorldPlayerController>(Player->GetController());
		if (PC)
		{
			RequestShowEventWidget(PC);
		}
	}
	else
	{
		UE_LOG(LogTemp,Warning,TEXT("상점을 이용할 수 있는 플레이어가 아닙니다."));
	}
	return;
}


bool AEventBase::CreateEventWidget()
{
	if (EventWidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShopkeeperBasePawn의 CreateShopWidget함수 ShopWidgetClass가 없습니다"));
		return false;
	}
	else
	{
		EventWidget = CreateWidget<UUserWidget>(GetWorld(), EventWidgetClass);
		return EventWidget ? true : false;
	}
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
	PC->ShowWidget(EventWidget, true);
}