// Fill out your copyright notice in the Description page of Project Settings.


#include "World/LevelActor/Shop/ShopkeeperBasePawn.h"

#include "Blueprint/UserWidget.h"
#include "Components/SphereComponent.h"
#include "World/WorldPlayerController.h"

// Sets default values
AShopkeeperBasePawn::AShopkeeperBasePawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>("ShopkeeperMesh");
	if (SkeletalMeshComponent == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("AShopkeeperBasePawn 클래스 생성자 : Failed to create SkeletalMeshComponent"));
	}
	
	ShopArea = CreateDefaultSubobject<USphereComponent>("ShopArea");
	ShopArea->SetupAttachment(SkeletalMeshComponent);
	if (ShopArea == nullptr)
	{ 
		UE_LOG(LogTemp, Error, TEXT("AShopkeeperBasePawn 클래스 생성자 : Failed to create USphereComponent"));
	}
}

// Called when the game starts or when spawned
void AShopkeeperBasePawn::BeginPlay()
{
	Super::BeginPlay();
	
	if (ShopArea)
	{
		ShopArea->OnComponentBeginOverlap.AddDynamic(this, &AShopkeeperBasePawn::OnAreaOverlap);
	}
	
	
}

void AShopkeeperBasePawn::OnAreaOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	RequestShowShopWidget();
}

// Called every frame
void AShopkeeperBasePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AShopkeeperBasePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}


bool AShopkeeperBasePawn::CreateShopWidget()
{
	if (ShopWidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShopkeeperBasePawn의 CreateShopWidget함수 ShopWidgetClass가 없습니다"));
		return false;
	}
	else
	{
		ShopWidget = CreateWidget<UUserWidget>(GetWorld(), ShopWidgetClass);
		return true;
	}
}

void AShopkeeperBasePawn::RequestShowShopWidget()
{
	if (ShopWidget == nullptr)
	{
		if (CreateShopWidget() == false)
		{
			return;
		}
	}
	
	//
	AWorldPlayerController* WorldPC = Cast<AWorldPlayerController>(GetWorld()->GetFirstPlayerController());
	
	if (WorldPC != nullptr)
	{
		WorldPC->ShowWidget(ShopWidget);
	}
}

