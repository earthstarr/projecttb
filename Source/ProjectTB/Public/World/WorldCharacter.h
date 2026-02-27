#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WorldCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * 월드(탐험) 씬 전용 3인칭 캐릭터.
 * 배틀 씬의 BattleCombatant와 완전히 분리.
 * Blueprint(BP_WorldCharacter)에서 메시·애니메이션을 설정.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTTB_API AWorldCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AWorldCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// ─── Enhanced Input ───────────────────────────────────────────────────────
	/** 월드 탐험 전용 입력 컨텍스트 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputMappingContext> WorldMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputAction> FreeLookAction;
	
	
	// ─── 입력 핸들러 ─────────────────────────────────────────────────────────
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleFreeLookStart();
	void HandleFreeLookEnd();
	
	// ─── 카메라 ──────────────────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	TObjectPtr<UCameraComponent> Camera;
	
	UPROPERTY(BlueprintReadOnly)
	FVector2D InputMoveAxis;

	UPROPERTY(BlueprintReadOnly)
	FVector2D InputLookAxis;
};
