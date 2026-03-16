// Fill out your copyright notice in the Description page of Project Settings.


#include "World/LevelActor/Potal/EventPotal.h"

#include "World/PotalManager.h"

void AEventPotal::SetEventType()
{
	UPotalManager* PotalManager = GetWorld()->GetSubsystem<UPotalManager>();
	if (PotalManager == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AEventPotal::SetEventType - PotalManager is nullptr"));
		return;
	}

	const TArray<FDataTableRowHandle>& Candidates = PotalManager->GetCachedEventRoomCandidates();
	if (Candidates.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("AEventPotal::SetEventType - EventRoomCandidates is empty"));
		return;
	}

	// 랜덤으로 하나 가져오기
	const int32 RandomIndex = FMath::RandRange(0, Candidates.Num() - 1);
	SelectedRoomHandle = Candidates[RandomIndex];
	
	UE_LOG(LogTemp, Log, TEXT("AEventPotal::SetEventType - Selected Row: %s"),
		*SelectedRoomHandle.RowName.ToString());
}

void AEventPotal::BeginPlay()
{
	Super::BeginPlay();
	
	SetEventType();
}

void AEventPotal::PotalActivate()
{
	if (SelectedRoomHandle.IsNull())
	{
		UE_LOG(LogTemp,Warning, TEXT("AEventPotal::PotalActivate - SelectedRoomHandle 가 비어있습니다. 임의로 생성합니다."));
		SetEventType();
	}
	
	Super::PotalActivate();
}



void AEventPotal::CleanRoom()
{
	Super::CleanRoom();
}
