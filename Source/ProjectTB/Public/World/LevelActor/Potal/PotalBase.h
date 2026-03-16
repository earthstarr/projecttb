// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PotalBase.generated.h"
class UBoxComponent;

UCLASS()
class PROJECTTB_API APotalBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APotalBase();
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// 다음 방 정보 설정. 각 클래스에서 재정의.
	virtual void SetNextRoom() {};
	
#pragma region Components
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Potal")
	UStaticMeshComponent* PotalMesh;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Potal")
	UBoxComponent* PotalBoxCollision;
#pragma endregion
	
	UFUNCTION()
	void OnPortalOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void PotalActivate();
	virtual void CleanRoom();
	
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Room Data")
	FDataTableRowHandle SelectedRoomHandle;
};
