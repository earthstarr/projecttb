// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "WorldAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTTB_API UWorldAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
protected:
	UPROPERTY(BlueprintReadOnly)
	class AWorldCharacterBase* OwningCharacter;
	
	UPROPERTY(BlueprintReadOnly)
	class UCharacterMovementComponent* OwingMovementComponent;
	
	UPROPERTY(BlueprintReadOnly, Category="Animation Cached Property")
	float GroundSpeedRight;
	
	UPROPERTY(BlueprintReadOnly, Category="Animation Cached Property")
	float GroundSpeedForward;
	
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Animation Cached Property")
	bool bHasAcceleration;
};
