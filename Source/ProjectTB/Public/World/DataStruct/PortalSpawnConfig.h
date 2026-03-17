// PortalSpawnConfig.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PortalSpawnConfig.generated.h"

class APotalBase;

UCLASS(BlueprintType)
class PROJECTTB_API UPortalSpawnConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Portal")
	TSoftClassPtr<APotalBase> BattlePortalClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Portal")
	TSoftClassPtr<APotalBase> EventPortalClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Portal")
	TSoftClassPtr<APotalBase> ShopPortalClass;
};