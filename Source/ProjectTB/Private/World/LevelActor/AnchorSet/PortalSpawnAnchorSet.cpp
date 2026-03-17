// Fill out your copyright notice in the Description page of Project Settings.


#include "World/LevelActor/AnchorSet/PortalSpawnAnchorSet.h"

#include "Engine/TargetPoint.h"
#include "World/PotalManager.h"

// Called when the game starts or when spawned
void APortalSpawnAnchorSet::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] AnchorSet BeginPlay - Name=%s Level=%s"),
		*GetName(),
		GetLevel() ? *GetLevel()->GetName() : TEXT("Null"));
	
	if (UPotalManager* PotalManager = GetWorld()->GetSubsystem<UPotalManager>())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] AnchorSet BeginPlay - RegisterPortalSpawnAnchorSet"));
		PotalManager->RegisterPortalSpawnAnchorSet(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] AnchorSet BeginPlay - PotalManager is null"));
	}
}

void APortalSpawnAnchorSet::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] AnchorSet EndPlay - Name=%s Reason=%d"),
		*GetName(), static_cast<int32>(EndPlayReason));

	if (UWorld* World = GetWorld())
	{
		if (UPotalManager* PotalManager = World->GetSubsystem<UPotalManager>())
		{
			UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] AnchorSet EndPlay - UnregisterPortalSpawnAnchorSet"));
			PotalManager->UnregisterPortalSpawnAnchorSet(this);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] AnchorSet EndPlay - PotalManager is null"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] AnchorSet EndPlay - World is null"));
	}

	Super::EndPlay(EndPlayReason);
}

void APortalSpawnAnchorSet::GetValidPortalSpawnPoints(TArray<ATargetPoint*>& OutPoints) const
{
	OutPoints.Reset();

	UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] AnchorSet GetValidPortalSpawnPoints - Name=%s ConfiguredPoints=%d"),
		*GetName(), PortalSpawnPoints.Num());

	for (ATargetPoint* Point : PortalSpawnPoints)
	{
		if (!IsValid(Point))
		{
			UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] AnchorSet GetValidPortalSpawnPoints - Invalid TargetPoint"));
			continue;
		}

		UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] AnchorSet GetValidPortalSpawnPoints - Valid TargetPoint=%s Level=%s"),
			*Point->GetName(),
			Point->GetLevel() ? *Point->GetLevel()->GetName() : TEXT("Null"));
		OutPoints.Add(Point);
	}

	UE_LOG(LogTemp, Warning, TEXT("[PortalDebug] AnchorSet GetValidPortalSpawnPoints - Result=%d"),
		OutPoints.Num());
}
