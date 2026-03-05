// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ShopkeeperBasePawn.generated.h"

class AWorldPlayerController;
class USphereComponent;

UCLASS()
class PROJECTTB_API AShopkeeperBasePawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AShopkeeperBasePawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

#pragma region Components
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shop")
	USkeletalMeshComponent* SkeletalMeshComponent;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Shop")
	USphereComponent* ShopArea;
#pragma endregion
	
	UFUNCTION()
	void OnAreaOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	
	
protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> ShopWidgetClass;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UUserWidget* ShopWidget;
	
	
private:
	bool CreateShopWidget();
	void RequestShowShopWidget(AWorldPlayerController* PC);
};
