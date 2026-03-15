// Fill out your copyright notice in the Description page of Project Settings.


#include "World/LevelActor/Potal/PotalBase.h"
#include "Components/BoxComponent.h"
#include "World/PotalManager.h"

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
	PotalActivate();
}

// Called every frame
void APotalBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APotalBase::PotalActivate()
{
	//레벨 로딩 요청
	UPotalManager* PotalManager = GetWorld()->GetSubsystem<UPotalManager>();
	PotalManager->OnLevelLoadStarted(SelectedRoomHandle);
}

void APotalBase::CleanRoom()
{
}


