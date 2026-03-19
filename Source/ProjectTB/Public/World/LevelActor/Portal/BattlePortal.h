// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TBGameInstance.h"
#include "World/DataStruct/RoomDataStruct.h"
#include "World/LevelActor/Portal/PortalBase.h"
#include "BattlePortal.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
/**
 * 
 */
UCLASS()
class PROJECTTB_API ABattlePortal : public APortalBase
{
	GENERATED_BODY()
	
public:
	ABattlePortal();
	virtual void BeginPlay();
	
	EBattleType BattleType;

	UFUNCTION(BlueprintCallable, Category="Portal VFX")
	void ApplyPortalNiagaraByTheme(const EThemeType Theme);
	
protected:
	virtual void PortalActivate() override;
	void CleanRoom() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Battle")
	FEnemyGroupData EnemyGroupData;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Battle")
	FBattleTransitionData BattleTransitionData;
	
	// 방 테마에 따른 포탈 나이아가라
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Portal VFX")
	TObjectPtr<UNiagaraComponent> PortalNiagaraComponent;
	
	UPROPERTY(EditDefaultsOnly, Category="Portal VFX")
	TMap<EThemeType, TSoftObjectPtr<UNiagaraSystem>> ThemeNiagaraMap;
	

	
};
