#include "World/LevelActor/Event/EventSpawner.h"

#include "Components/SceneComponent.h"
#include "World/LevelActor/Event/EventBase.h"

AEventSpawner::AEventSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	SpawnAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnAnchor"));
	SpawnAnchor->SetupAttachment(Root);
}

void AEventSpawner::BeginPlay()
{
	Super::BeginPlay();

	SpawnRandomEvent();
}

void AEventSpawner::SpawnRandomEvent()
{
	if (SpawnedEvent != nullptr)
	{
		return;
	}

	TArray<TSubclassOf<AEventBase>> ValidCandidates;
	for (const TSubclassOf<AEventBase>& Candidate : EventCandidates)
	{
		if (*Candidate != nullptr)
		{
			ValidCandidates.Add(Candidate);
		}
	}

	if (ValidCandidates.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("AEventSpawner::SpawnRandomEvent - EventCandidates is empty"));
		return;
	}

	const int32 RandomIndex = FMath::RandRange(0, ValidCandidates.Num() - 1);
	const TSubclassOf<AEventBase> SelectedClass = ValidCandidates[RandomIndex];

	const FTransform SpawnTransform =
		SpawnAnchor ? SpawnAnchor->GetComponentTransform() : GetActorTransform();

	FActorSpawnParameters SpawnParams;
	SpawnParams.OverrideLevel = GetLevel();
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = SpawnCollisionHandling;

	SpawnedEvent = GetWorld()->SpawnActor<AEventBase>(
		SelectedClass,
		SpawnTransform,
		SpawnParams);

	if (SpawnedEvent == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AEventSpawner::SpawnRandomEvent - failed to spawn event"));
	}
}