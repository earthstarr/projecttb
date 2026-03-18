// PortalSpawnConfig.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PortalSpawnConfig.generated.h"

class APortalBase;

UCLASS(BlueprintType)
class PROJECTTB_API UPortalSpawnConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Portal")
	TSoftClassPtr<APortalBase> BattlePortalClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Portal")
	TSoftClassPtr<APortalBase> EventPortalClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Portal")
	TSoftClassPtr<APortalBase> ShopPortalClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Portal")
	TSoftClassPtr<APortalBase> BossPreparationPortalClass;
};