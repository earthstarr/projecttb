// Fill out your copyright notice in the Description page of Project Settings.


#include "World/LevelActor/Potal/PotalBase.h"
#include "Components/BoxComponent.h"
#include "World/PotalManager.h"
#include "TBGameInstance.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
APotalBase::APotalBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	PotalMesh = CreateDefaultSubobject<UStaticMeshComponent>("PotalMesh");
	if (PotalMesh == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("APotalBase 클래스 생성자 : Failed to create StaticMeshComponent"));
	}
	
	PotalBoxCollision = CreateDefaultSubobject<UBoxComponent>("PotalBoxCollision");
	PotalBoxCollision->SetupAttachment(PotalMesh);
	if (PotalBoxCollision == nullptr)
	{ 
		UE_LOG(LogTemp, Error, TEXT("APotalBase 클래스 생성자 : Failed to create BoxComponent"));
	}
}

void APotalBase::InitializePortal(const FDataTableRowHandle& InSelectedRoomHandle)
{
	SelectedRoomHandle = InSelectedRoomHandle;
}

// Called when the game starts or when spawned
void APotalBase::BeginPlay()
{
	Super::BeginPlay();
	
	if (SelectedRoomHandle.IsNull())
	{
		UE_LOG(LogTemp,Warning,TEXT("Potal의 SelectedRoomHandle이 없습니다."));
	}
	
	if (PotalBoxCollision)
	{
		PotalBoxCollision->OnComponentBeginOverlap.AddDynamic(this, &APotalBase::OnPortalOverlap);
	}
	
}

void APotalBase::OnPortalOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
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
	PotalActivate();
}

// Called every frame
void APotalBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APotalBase::PotalActivate()
{
	if (SelectedRoomHandle.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("APotalBase::PotalActivate - SelectedRoomHandle is null."));
		return;
	}

	UPotalManager* PotalManager = GetWorld()->GetSubsystem<UPotalManager>();
	if (PotalManager == nullptr)
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
	
	//레벨 로딩 요청
	PotalManager->OnLevelLoadStarted(SelectedRoomHandle);
}

void APotalBase::CleanRoom()
{
}


