#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BattleImpactActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UProjectileMovementComponent;
class ABattleCombatant;
class UTBGameplayAbility;
class UAudioComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBattleImpact);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBattleImpactFinished);

/**
 * 몽타주 종료 후 스폰되어 시각 이펙트와 데미지 타이밍을 관리하는 액터.
 * Blueprint 서브클래스에서 NiagaraComponent 등 비주얼을 추가한다.
 *
 * 흐름: BeginPlay → ImpactDelay 후 OnImpact 발생 (데미지) → TotalDuration 후 OnFinished 발생 (어빌리티 종료)
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTTB_API ABattleImpactActor : public AActor
{
	GENERATED_BODY()

public:
	ABattleImpactActor();

	// 타겟 위치 기준 스폰 오프셋 (운석: Z=500 등 하늘에서 시작)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Impact")
	FVector SpawnOffset = FVector::ZeroVector;
	
	// 날아갈 방향 (기본값은 전방 X축)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact")
	FVector ShootDirection = FVector(1.0f, 0.0f, 0.0f);

	// Impact 시점에 이동 정지 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Impact")
	bool bStopOnImpact = true;

	// Impact 시점에 MeshComponent 숨기기 (Ranged 발사체처럼 메시가 사라지고 Niagara만 재생할 때)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Impact")
	bool bHideMeshOnImpact = false;

	// 스폰 후 데미지 발생까지 딜레이 (초) — 운석이 땅에 닿는 시점 등
	// bAutoCalculateImpactDelay = true이면 런타임에 덮어씀
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Impact")
	float ImpactDelay = 1.0f;

	// true면 InitialSpeed + TargetLocation으로 ImpactDelay 자동 계산 (Ranged 타입용)
	// SpawnSingleImpactActor에서 Ranged일 때 자동 설정
	UPROPERTY(BlueprintReadWrite, Category="Impact")
	bool bAutoCalculateImpactDelay = false;

	// 자동 계산 시 타겟 위치. SpawnSingleImpactActor에서 자동 주입됨
	UPROPERTY(BlueprintReadWrite, Category="Impact")
	FVector TargetLocation = FVector::ZeroVector;

	// 패링 가능 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Impact|Parry")
	bool bParryable = false;

	// 데미지 발생 몇 초 전에 패링 타이밍을 열지 (ImpactDelay보다 작아야 함)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Impact|Parry")
	float ParryTimingLeadTime = 0.5f;

	// 이펙트 전체 재생 시간 (초) — 이 시간 후 OnFinished 발생 및 액터 제거
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Impact")
	float TotalDuration = 2.0f;

	// ImpactDelay 후 발생 → TBGameplayAbility::ApplyDamage 호출용
	UPROPERTY(BlueprintAssignable, Category="Impact")
	FOnBattleImpact OnImpact;

	// TotalDuration 후 발생 → TBGameplayAbility::FinishAbility 호출용
	UPROPERTY(BlueprintAssignable, Category="Impact")
	FOnBattleImpactFinished OnFinished;

	// ─── 비주얼 컴포넌트 (Blueprint에서 메시/이펙트 설정) ────────────────────
	// 루트 컴포넌트 (이동/스폰 위치 기준)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<USceneComponent> RootSceneComponent;

	// MeshComponent: 뷰포트에서 회전/스케일 직접 편집 가능
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<UNiagaraComponent> ImpactNiagaraComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<UNiagaraComponent> TrailNiagaraComponent;

	// Impact 시점에 재생할 Niagara 이펙트
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Visual")
	TObjectPtr<UNiagaraSystem> ImpactEffect;
	
	// Niagara 트레일
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<UNiagaraComponent> TrailEffect;
	
	// 초기 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Impact")
	float InitialSpeed = 0.0f;
	
	// 최대 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Impact")
	float MaxSpeed = 0.0f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movement")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
	
	// ──────────── 사운드 ────────────────────────────────────
	
	UPROPERTY()
	TObjectPtr<class UAudioComponent> FlyingLoopAudioComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Sound")
	TObjectPtr<USoundBase> SpawnSound;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Sound")
	TObjectPtr<USoundBase> FlyingLoopSound;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Sound")
	TObjectPtr<USoundBase> ImpactSound;

	// ─── 개별 타겟 모드 (bSpawnImpactPerTarget 용) ────────────────────────────
	// true면 OnImpact 대신 OwningAbility->ApplyDamageToSingleTarget(TargetCombatant) 직접 호출
	bool bPerTargetMode = false;

	UPROPERTY()
	TWeakObjectPtr<ABattleCombatant> TargetCombatant;

	UPROPERTY()
	TWeakObjectPtr<UTBGameplayAbility> OwningAbility;

	int32 PerTargetHitIndex = 0;

	// TBGameplayAbility::SpawnSingleImpactActor에서 호출
	void SetPerTargetMode(UTBGameplayAbility* InAbility, ABattleCombatant* InTarget, int32 InHitIndex);
	friend class UTBGameplayAbility;

protected:
	virtual void BeginPlay() override;

	// Impact 시점에 이펙트 재생 (Blueprint에서 오버라이드 가능)
	UFUNCTION(BlueprintNativeEvent, Category="Impact")
	void PlayImpactEffect();
	virtual void PlayImpactEffect_Implementation();

private:
	UFUNCTION() void TriggerImpact();
	UFUNCTION() void TriggerFinished();
	UFUNCTION() void TriggerOpenParryTiming();

	FTimerHandle ImpactTimer;
	FTimerHandle FinishedTimer;
	FTimerHandle ParryOpenTimer;
};
