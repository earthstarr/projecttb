#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EventSpawner.generated.h"

class AEventBase;
class USceneComponent;

UCLASS()
class PROJECTTB_API AEventSpawner : public AActor
{
	GENERATED_BODY()

public:
	AEventSpawner();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spawner")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spawner")
	TObjectPtr<USceneComponent> SpawnAnchor;

	// BP_Event_ArtifactsGift, BP_Event_DamageAndDice 같은 후보 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner")
	TArray<TSubclassOf<AEventBase>> EventCandidates;

	UPROPERTY(BlueprintReadOnly, Category="Spawner")
	TObjectPtr<AEventBase> SpawnedEvent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawner")
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandling = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

private:
	void SpawnRandomEvent();
};
