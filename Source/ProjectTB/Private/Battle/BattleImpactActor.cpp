
#include "Battle/BattleImpactActor.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"

ABattleImpactActor::ABattleImpactActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// 루트 컴포넌트로 StaticMesh 생성 (Blueprint에서 메시 할당)
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	
	// 트레일 이펙트
	TrailNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailNiagaraComponent"));
	TrailNiagaraComponent->SetupAttachment(MeshComponent);
	TrailNiagaraComponent->bAutoActivate = true; // 스폰되자마자 바로 실행

	// Niagara 컴포넌트 (Blueprint에서 이펙트 설정 또는 ImpactEffect 사용)
	ImpactNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	ImpactNiagaraComponent->SetupAttachment(MeshComponent);
	ImpactNiagaraComponent->bAutoActivate = false; // Impact 시점에 수동 활성화
	
	// 발사체 컴포넌트 생성
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    
	// 기본 설정: UpdateComponent를 MeshComponent로 지정
	ProjectileMovement->SetUpdatedComponent(MeshComponent);
    
	// 초기 속도값 대입
	ProjectileMovement->InitialSpeed = InitialSpeed;
	ProjectileMovement->MaxSpeed = MaxSpeed;
    
	// 중력
	ProjectileMovement->ProjectileGravityScale = 0.0f;
}

void ABattleImpactActor::BeginPlay()
{
	// 스폰 시점에 변수값이 반영되도록 다시 설정 (Blueprint 수정값 반영용)
	if (ProjectileMovement)
	{
		FVector Dir = ShootDirection;
		
		Dir.Normalize();		
		ProjectileMovement->Velocity = Dir * InitialSpeed;
		ProjectileMovement->UpdateComponentVelocity();
	}
	
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(
		ImpactTimer, this, &ABattleImpactActor::TriggerImpact, ImpactDelay, false);

	GetWorldTimerManager().SetTimer(
		FinishedTimer, this, &ABattleImpactActor::TriggerFinished, TotalDuration, false);
}

void ABattleImpactActor::TriggerImpact()
{
	PlayImpactEffect();
	OnImpact.Broadcast();
}

void ABattleImpactActor::PlayImpactEffect_Implementation()
{
	// 폭발 시점에 트레일 이펙트 중지
	if (TrailNiagaraComponent)
	{
		TrailNiagaraComponent->Deactivate(); 
	}
	
	// ImpactEffect가 설정된 경우 해당 위치에 스폰
	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), ImpactEffect, GetActorLocation(), GetActorRotation());
	}
	// NiagaraComponent에 직접 이펙트가 설정된 경우 활성화
	else if (ImpactNiagaraComponent && ImpactNiagaraComponent->GetAsset())
	{
		ImpactNiagaraComponent->Activate(true);
	}
}

void ABattleImpactActor::TriggerFinished()
{
	OnFinished.Broadcast();
	Destroy();
}
