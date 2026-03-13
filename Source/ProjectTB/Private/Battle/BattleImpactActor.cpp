#include "Battle/BattleImpactActor.h"
#include "Battle/BattleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Battle/BattleCombatant.h"
#include "Abilities/TBGameplayAbility.h"

ABattleImpactActor::ABattleImpactActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// 루트 컴포넌트: SceneComponent (이동/스폰 위치 기준)
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	SetRootComponent(RootSceneComponent);

	// MeshComponent: 루트의 자식으로 생성 → 뷰포트에서 회전/스케일 편집 가능
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootSceneComponent);

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

	// 기본 설정: UpdateComponent를 RootSceneComponent로 지정 (액터 전체 이동)
	ProjectileMovement->SetUpdatedComponent(RootSceneComponent);

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

	// Ranged 타입: 거리/속도로 ImpactDelay 자동 계산, 패링 타이밍 0.2초 전으로 고정
	if (bAutoCalculateImpactDelay && InitialSpeed > 0.f)
	{
		const float Distance = FVector::Dist(GetActorLocation(), TargetLocation);
		ImpactDelay = Distance / InitialSpeed;
		ParryTimingLeadTime = 0.2f;
	}

	GetWorldTimerManager().SetTimer(
		ImpactTimer, this, &ABattleImpactActor::TriggerImpact, ImpactDelay, false);

	GetWorldTimerManager().SetTimer(
		FinishedTimer, this, &ABattleImpactActor::TriggerFinished, TotalDuration, false);

	// 패링 가능한 경우: 데미지 발생 ParryTimingLeadTime 초 전에 타이밍 열기
	if (bParryable && ParryTimingLeadTime > 0.f)
	{
		const float ParryOpenTime = FMath::Max(0.01f, ImpactDelay - ParryTimingLeadTime);
		GetWorldTimerManager().SetTimer(
			ParryOpenTimer, this, &ABattleImpactActor::TriggerOpenParryTiming, ParryOpenTime, false);
	}
}

void ABattleImpactActor::TriggerImpact()
{
	// Impact 시점에 이동 정지
	if (bStopOnImpact && ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
	}

	if (bHideMeshOnImpact && MeshComponent)
		MeshComponent->SetVisibility(false);

	PlayImpactEffect();

	// 개별 타겟 모드: OnImpact 대신 직접 단일 타겟에만 데미지 적용
	if (bPerTargetMode && OwningAbility.IsValid() && TargetCombatant.IsValid())
	{
		OwningAbility->ApplyDamageToSingleTarget(TargetCombatant.Get(), PerTargetHitIndex);
	}
	else
	{
		OnImpact.Broadcast();
	}
}

void ABattleImpactActor::SetPerTargetMode(UTBGameplayAbility* InAbility, ABattleCombatant* InTarget, int32 InHitIndex)
{
	bPerTargetMode = true;
	OwningAbility = InAbility;
	TargetCombatant = InTarget;
	PerTargetHitIndex = InHitIndex;
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

void ABattleImpactActor::TriggerOpenParryTiming()
{
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABattleManager::StaticClass(), Found);
	if (!Found.IsEmpty())
	{
		if (ABattleManager* BM = Cast<ABattleManager>(Found[0]))
			BM->OpenParryTiming(ParryTimingLeadTime);
	}
}

void ABattleImpactActor::TriggerFinished()
{
	OnFinished.Broadcast();
	Destroy();
}
