// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PortalBase.generated.h"
class UBoxComponent;

UCLASS()
class PROJECTTB_API APortalBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APortalBase();
	
	// 포탈 매니저에서 주는 핸들로 방 초기화
	virtual void InitializePortal(const FDataTableRowHandle& InSelectedRoomHandle);
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// 다음 방 정보 설정. 각 클래스에서 재정의.
	virtual void SetNextRoom() {};
	
#pragma region Components
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Portal")
	UStaticMeshComponent* PortalMesh;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Portal")
	UBoxComponent* PortalBoxCollision;
#pragma endregion
	
	UFUNCTION()
	void OnPortalOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void PortalActivate();
	virtual void CleanRoom();
	
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Room Data")
	FDataTableRowHandle SelectedRoomHandle;
	
private:
	bool bOverlapped = false;
};
