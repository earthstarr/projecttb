// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ShopkeeperBasePawn.generated.h"

struct FArtifactEntry;
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

	// 판매할 상품 목록 초기화 여부
	UPROPERTY()
	bool bShopInventoryInitialized = false;
	
	// 상품 목록 초기화
	UFUNCTION(BlueprintCallable, Category="Shop")
	void InitializeShopInventory();

	// 판매할 상품 아이템 선별
	UFUNCTION(BlueprintCallable, Category="Shop")
	const TArray<FArtifactEntry>& GetShopProductEntries() const;
	
	UFUNCTION(BlueprintCallable, Category="Shop")
	bool IsSoldOut(FName ArtifactID) const;

	UFUNCTION(BlueprintCallable, Category="Shop")
	void MarkProductSoldOut(FName ArtifactID);
	
protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "UI")
	TSubclassOf<UUserWidget> ShopWidgetClass;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UUserWidget* ShopWidget;
	
	// 판매할 상품 목록
	UPROPERTY(BlueprintReadOnly, Category="Shop")
	TArray<FArtifactEntry> ShopProductEntries;
	
	// 판매된 아티팩트 이름
	UPROPERTY()
	TSet<FName> SoldOutArtifactIds;
	
private:
	bool CreateShopWidget();
	void RequestShowShopWidget(AWorldPlayerController* PC);
};
