#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "TBGameplayTags.h"
#include "TBGameplayAbility.generated.h"

class UNiagaraSystem;

UENUM(BlueprintType)
enum class EAbilityCostType : uint8
{
	None,
	Stamina,
	MP,
	HP
};

UENUM(BlueprintType)
enum class EAbilityTargetType : uint8
{
	Self,
	SingleEnemy,
	AllEnemies,
	SingleAlly,
	AllAllies
};

UENUM(BlueprintType)
enum class EAbilityAnimType : uint8
{
	Melee,   // 캐릭터가 타겟에게 이동 후 공격, 복귀
	Ranged,  // 제자리 캐스팅 + AnimNotify 데미지
	Impact   // 몽타주 종료 후 BattleImpactActor 스폰 → 내부 타이머로 데미지/종료
};

class ABattleImpactActor;
class ABattleManager;
class ABattleCombatant;

// ─── 어빌리티 부여 상태이상 설정 ─────────────────────────────────────────────
// Blueprint에서 OnHitStatusEffects[]에 추가해 어빌리티마다 상태이상 설정.
USTRUCT(BlueprintType)
struct FStatusEffectConfig
{
	GENERATED_BODY()

	// 부여할 상태이상 태그 (Status.Burn / Status.Poison / Status.Regen / Status.Stun)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag StatusTag;

	// 추가할 스택 수 (= 지속 턴 수)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="1"))
	int32 StacksToApply = 1;

	// 시전자 MagicAttack 배율 (틱당 데미지/힐 = CasterMagicAttack × ScalingCoeff)
	// Stun은 0으로 설정. Poison은 HP% 부분이 틱마다 자동 추가됨.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float ScalingCoeff = 0.3f;
};

/**
 * 모든 배틀 어빌리티의 기반 클래스.
 * 개별 어빌리티는 Blueprint에서 이 클래스를 상속해 만든다.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PROJECTTB_API UTBGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UTBGameplayAbility();

	// ─── UI 표시 정보 ────────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display")
	FText AbilityDisplayName;

	// false로 설정하면 어빌리티 메뉴에 표시되지 않음 (기본 공격 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display")
	bool bShowInAbilityMenu = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display")
	FText AbilityDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Display")
	TObjectPtr<UTexture2D> AbilityIcon;

	// ─── 비용 (UI 표시용 — 실제 차감은 CostGameplayEffectClass로) ──────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cost")
	EAbilityCostType CostType = EAbilityCostType::Stamina;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cost")
	float CostAmount = 0.f;

	// ─── 타겟팅 ──────────────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Targeting")
	EAbilityTargetType TargetType = EAbilityTargetType::SingleEnemy;

	// ─── 애니메이션 ──────────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	EAbilityAnimType AnimationType = EAbilityAnimType::Melee;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UAnimMontage> AbilityMontage;

	// Melee 전용: 타겟까지 이동 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation", meta=(EditCondition="AnimationType==EAbilityAnimType::Melee"))
	float MoveSpeed = 600.f;

	// ─── 데미지 ──────────────────────────────────────────────────────────────
	// 설정하면 ActivateAbility를 Blueprint 없이 C++에서 자동 처리
	// 비워두면 Blueprint의 Event ActivateAbility가 실행됨
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 타격별 데미지 배율 (AnimNotify_OnHit의 HitIndex와 대응)
	// 단타: [1.0] / 3연타 예시: [0.5, 0.5, 1.0]
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage")
	TArray<float> HitMultipliers = {1.0f};

	// Physical / Magic 태그 (TBDamageExecution에서 읽음, GE 스펙 소스 태그에 주입)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage")
	FGameplayTag AbilityTypeTag;

	// Ranged/Impact: 스폰할 BattleImpactActor 클래스
	// Ranged: AnimNotify_SpawnImpact 타이밍에 스폰 / Impact: 몽타주 종료 후 자동 스폰
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage", meta=(EditCondition="AnimationType!=EAbilityAnimType::Melee"))
	TSubclassOf<ABattleImpactActor> ImpactActorClass;

	// 노티파이로부터 호출받을 인터페이스
	void RequestSpawnImpact(int32 HitIndex = 0);

	// bSpawnImpactPerTarget일 때 개별 타겟에만 데미지 적용 (ImpactActor에서 호출)
	void ApplyDamageToSingleTarget(ABattleCombatant* Target, int32 HitIndex);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Impact")
	bool bUseFixedWorldRotation = true; // Impact일 때 기본적으로 켬

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Impact", meta=(EditCondition="bUseFixedWorldRotation"))
	FRotator FixedWorldRotation = FRotator(-90.f, 0.f, 0.f); // 기본값: 수직 하강

	// ─── 전체 공격(AllEnemies/AllAllies) 이펙트 스폰 방식 ──────────────────────
	// true: 각 타겟마다 ImpactActor 스폰 / false: 모든 타겟의 중간 위치에 하나만 스폰
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Impact")
	bool bSpawnImpactPerTarget = false;

	// ─── Hit 이펙트 (Melee/Ranged 타격 시 플레이어 위치에 스폰) ─────────────────
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Visual")
	TObjectPtr<UNiagaraSystem> HitEffect;

	// Hit 이펙트 스폰 오프셋 (플레이어/시전자 기준)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Visual")
	FVector HitEffectOffset = FVector::ZeroVector;

	// AnimNotify_AbilityEffect에서 호출 — 시전자 위치에 HitEffect 스폰
	UFUNCTION(BlueprintCallable, Category="Visual")
	void SpawnHitEffect();

	// ─── 액션 카메라 ──────────────────────────────────────────────────────────
	// false면 기본 BattleCamera 유지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera")
	bool bUseActionCamera = false;

	// 시전 캐릭터 로컬 기준 카메라 위치/회전
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera", meta=(EditCondition="bUseActionCamera"))
	FVector ActionCameraLocalOffset = FVector(-250.f, 80.f, 100.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera", meta=(EditCondition="bUseActionCamera"))
	FRotator ActionCameraLocalRotation = FRotator(-15.f, 0.f, 0.f);

	// 액션 카메라로 전환할 때 블렌드 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera", meta=(EditCondition="bUseActionCamera"))
	float CameraBlendInTime = 0.3f;

	// 기본 카메라로 복귀할 때 블렌드 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera", meta=(EditCondition="bUseActionCamera"))
	float CameraBlendOutTime = 0.5f;

	// ─── 몽타주 카메라 (몽타주 재생 중 카메라) ─────────────────────────────────
	// bUseActionCamera가 true일 때만 활성화 가능
	// 몽타주 재생 동안 이 카메라 위치에서 촬영
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera|Montage", meta=(EditCondition="bUseActionCamera"))
	bool bUseMontageCamera = false;

	// 몽타주 카메라 위치 (시전자 로컬 기준)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera|Montage", meta=(EditCondition="bUseMontageCamera"))
	FVector MontageCameraLocalOffset = FVector(-200.f, 100.f, 100.f);

	// 몽타주 카메라 회전 (시전자 로컬 기준)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera|Montage", meta=(EditCondition="bUseMontageCamera"))
	FRotator MontageCameraLocalRotation = FRotator(-10.f, 0.f, 0.f);

	// 몽타주 카메라 전환 블렌드 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera|Montage", meta=(EditCondition="bUseMontageCamera"))
	float MontageCameraBlendInTime = 0.3f;

	// ─── 컷씬 카메라 (궁극기 연출용) ─────────────────────────────────────────
	// bUseActionCamera가 true일 때만 활성화 가능
	// 몽타주 종료 후 컷씬 카메라로 전환, 컷씬 종료 후 액션 카메라로 전환
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera|Cutscene", meta=(EditCondition="bUseActionCamera"))
	bool bUseCutsceneCamera = false;

	// 컷씬 카메라 위치 (시전자 로컬 기준)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera|Cutscene", meta=(EditCondition="bUseCutsceneCamera"))
	FVector CutsceneCameraLocalOffset = FVector(0.f, 0.f, 500.f);

	// 컷씬 카메라 회전 (시전자 로컬 기준)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera|Cutscene", meta=(EditCondition="bUseCutsceneCamera"))
	FRotator CutsceneCameraLocalRotation = FRotator(-90.f, 0.f, 0.f);

	// 컷씬 카메라 유지 시간 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera|Cutscene", meta=(EditCondition="bUseCutsceneCamera"))
	float CutsceneDuration = 2.0f;

	// 컷씬 전환 블렌드 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera|Cutscene", meta=(EditCondition="bUseCutsceneCamera"))
	float CutsceneBlendInTime = 0.3f;

	// ─── 컷씬 Niagara ────────────────────────────────────────────────────────
	// 컷씬 중 재생할 Niagara 이펙트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera|Cutscene", meta=(EditCondition="bUseCutsceneCamera"))
	TObjectPtr<UNiagaraSystem> CutsceneNiagaraEffect;

	// Niagara 스폰 위치 (시전자 로컬 기준)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera|Cutscene", meta=(EditCondition="bUseCutsceneCamera"))
	FVector CutsceneNiagaraLocalOffset = FVector(0.f, 0.f, 500.f);

	// Niagara 스폰 딜레이 (컷씬 시작 후 몇 초 뒤)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera|Cutscene", meta=(EditCondition="bUseCutsceneCamera"))
	float CutsceneNiagaraDelay = 0.5f;

	// ─── 상태이상 부여 설정 ──────────────────────────────────────────────────
	// 이 어빌리티가 타격 시 대상에게 부여할 상태이상 목록.
	// Blueprint에서 편집: {StatusTag=Status.Burn, StacksToApply=2, ScalingCoeff=0.4}
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Status Effects")
	TArray<FStatusEffectConfig> OnHitStatusEffects;

	// ─── 유틸 ────────────────────────────────────────────────────────────────
	// UI에서 비용 지불 가능 여부 확인 (그레이아웃 처리용)
	UFUNCTION(BlueprintCallable, Category="Ability")
	bool CanAffordCost(UAbilitySystemComponent* ASC) const;

	// AnimNotify_OnHit → BattleCombatant::OnHitNotify → 여기서 호출
	// HitIndex: HitMultipliers 배열 인덱스 (0부터 시작)
	void ApplyDamage(int32 HitIndex);

protected:
	// GAS 비용 체크 override — CanAffordCost를 실제 활성화 전 검증에 연결
	virtual bool CheckCost(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION() void OnMontageCompleted();
	UFUNCTION() void OnMontageBlendOut();
	
	// Impact: BattleImpactActor 델리게이트 수신
	UFUNCTION() void OnImpactNotify();
	UFUNCTION() void OnImpactFinished();

	void SpawnImpactActor();

	// 단일 타겟에 ImpactActor 스폰 (전체 공격에서 개별 스폰 시 사용)
	void SpawnSingleImpactActor(AActor* Avatar, class ABattleCombatant* Target, const ABattleImpactActor* CDO, bool bBindFinished, bool bPerTargetMode = false);

	// 특정 위치에 ImpactActor 스폰 (전체 공격에서 중앙 스폰 시 사용)
	void SpawnSingleImpactActorAtLocation(AActor* Avatar, const FVector& Location, const ABattleImpactActor* CDO, bool bBindFinished);

	// 어빌리티 종료 처리 (원위치 복귀 + OnActionComplete + EndAbility)
	void FinishAbility();

	ABattleManager* GetBattleManager() const;

	bool bAbilityFinished = false;
	bool bImpactSpawned = false;
	FVector OriginLocation = FVector::ZeroVector;
	FRotator OriginRotation; // 원래 회전값을 저장할 변수

	FTimerHandle PreMoveTimer;
	FTimerHandle PreMontageTimer;
	FTimerHandle CutsceneTimer;
	FTimerHandle CutsceneNiagaraTimer;
	FTimerHandle CutsceneCameraBlendTimer;
	FTimerHandle MontageCameraBlendTimer;

	int32 PendingHitIndex = 0;

	UFUNCTION() void DoTeleportToTarget();
	UFUNCTION() void DoPlayMontage();
	UFUNCTION() void OnMontageCameraBlendFinished();
	void PlayMontageInternal();

	// 컷씬 카메라 관련
	UFUNCTION() void StartCutsceneCamera();
	UFUNCTION() void OnCutsceneCameraBlendFinished();
	UFUNCTION() void SpawnCutsceneNiagara();
	UFUNCTION() void OnCutsceneFinished();

	// 액션 카메라로 전환 후 ImpactActor 스폰 (컷씬 없이 몽타주 카메라만 사용할 때)
	void SwitchToActionCameraAndSpawnImpact();
};
