// Fill out your copyright notice in the Description page of Project Settings.


#include "World/LevelActor/Shop/ShopkeeperBasePawn.h"

#include "TBGameInstance.h"
#include "Blueprint/UserWidget.h"
#include "Components/SphereComponent.h"
#include "UI/World/ShopWidget.h"
#include "World/WorldCharacter.h"
#include "World/WorldCharacterBase.h"
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
	
	//상품 목록 설정
	InitializeShopInventory();
	
	if (ShopArea)
	{
		ShopArea->OnComponentBeginOverlap.AddDynamic(this, &AShopkeeperBasePawn::OnAreaOverlap);
	}
}

void AShopkeeperBasePawn::OnAreaOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//이 부분 GAS로 상점을 이용할 수 있는지 확인 하면 좋을듯 하다. todo
	AWorldCharacterBase* Player = Cast<AWorldCharacterBase>(OtherActor);
	if (Player)
	{
		AWorldPlayerController* PC = Cast<AWorldPlayerController>(Player->GetController());
		if (PC)
		{
			RequestShowShopWidget(PC);
		}
	}
	else
	{
		UE_LOG(LogTemp,Warning,TEXT("상점을 이용할 수 있는 플레이어가 아닙니다."));
	}
	return;
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


void AShopkeeperBasePawn::InitializeShopInventory()
{
	if (bShopInventoryInitialized)
	{
		return;
	}

	UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance());
	if (GI == nullptr)
	{
		return;
	}

	ShopProductEntries = GI->GetUnownedArtifacts();

	// 보스, 이벤트 등급은 제외
	ShopProductEntries.RemoveAll([](const FArtifactEntry& Entry)
	{
		return Entry.ArtifactData.Grade == EArtifactGrade::Boss
			|| Entry.ArtifactData.Grade == EArtifactGrade::Event;
	});

	bShopInventoryInitialized = true;
}

const TArray<FArtifactEntry>& AShopkeeperBasePawn::GetShopProductEntries() const
{
	return ShopProductEntries;
}

bool AShopkeeperBasePawn::IsSoldOut(FName ArtifactID) const
{
	return SoldOutArtifactIds.Contains(ArtifactID);
}

void AShopkeeperBasePawn::MarkProductSoldOut(FName ArtifactID)
{
	SoldOutArtifactIds.Add(ArtifactID);
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
		if (UShopWidget* TypedShopWidget = Cast<UShopWidget>(ShopWidget))
		{
			TypedShopWidget->InitializeShop(this);
		}
		return true;
	}
}

void AShopkeeperBasePawn::RequestShowShopWidget(AWorldPlayerController* PC)
{
	if (ShopWidget == nullptr)
	{
		if (CreateShopWidget() == false)
		{
			return;
		}
	}
	PC->ShowWidget(ShopWidget, true);
}
