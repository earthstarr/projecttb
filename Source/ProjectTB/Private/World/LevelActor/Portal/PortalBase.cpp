// Fill out your copyright notice in the Description page of Project Settings.


#include "World/LevelActor/Portal/PortalBase.h"
#include "Components/BoxComponent.h"
#include "World/PortalManager.h"
#include "TBGameInstance.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
APortalBase::APortalBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>("PortalMesh");
	if (PortalMesh == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("APortalBase 클래스 생성자 : Failed to create StaticMeshComponent"));
	}
	
	PortalBoxCollision = CreateDefaultSubobject<UBoxComponent>("PortalBoxCollision");
	PortalBoxCollision->SetupAttachment(PortalMesh);
	if (PortalBoxCollision == nullptr)
	{ 
		UE_LOG(LogTemp, Error, TEXT("APortalBase 클래스 생성자 : Failed to create BoxComponent"));
	}
}

void APortalBase::InitializePortal(const FDataTableRowHandle& InSelectedRoomHandle)
{
	SelectedRoomHandle = InSelectedRoomHandle;
}

// Called when the game starts or when spawned
void APortalBase::BeginPlay()
{
	Super::BeginPlay();
	
	if (SelectedRoomHandle.IsNull())
	{
		UE_LOG(LogTemp,Warning,TEXT("Portal의 SelectedRoomHandle이 없습니다."));
	}
	
	if (PortalBoxCollision)
	{
		PortalBoxCollision->OnComponentBeginOverlap.AddDynamic(this, &APortalBase::OnPortalOverlap);
	}
	
}

void APortalBase::OnPortalOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bOverlapped)
	{
		return;
	}
	
	if (OtherActor == nullptr || OtherActor != UGameplayStatics::GetPlayerPawn(this, 0))
	{
		return;
	}

	if (SelectedRoomHandle.IsNull())
	{
		return;
	}
	
	bOverlapped = true;
	PortalActivate();
}

// Called every frame
void APortalBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APortalBase::PortalActivate()
{
	if (SelectedRoomHandle.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("APortalBase::PortalActivate - SelectedRoomHandle is null."));
		return;
	}

	UPortalManager* PortalManager = GetWorld()->GetSubsystem<UPortalManager>();
	if (PortalManager == nullptr)
	{
		return;
	}
	
	//포탈 이동 했으므로 난이도 (층) 상승
	UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance());
	if (GI != nullptr)
	{
		GI->IncreasePortalMoveCount();
		UE_LOG(LogTemp, Log, TEXT("PortalMoveCount: %d"), GI->GetPortalMoveCount());
	}
	
	// 포탈 이동 시 파티 HP/MP를 회복한 뒤 레벨 로딩 요청
	PortalManager->OnPortalTravelStarted(SelectedRoomHandle);
}

void APortalBase::CleanRoom()
{
}


