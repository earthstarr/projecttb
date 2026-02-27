// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Animation/WorldAnimInstance.h"
#include "World/WorldCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"

void UWorldAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	OwningCharacter = Cast<AWorldCharacterBase>(TryGetPawnOwner());
	if (OwningCharacter)
	{
		OwingMovementComponent = OwningCharacter->GetCharacterMovement();
	}
	else
	{
		UE_LOG(LogTemp,Warning,TEXT("WorldAnimInstance.cpp의 NativeInitializeAnimation 함수에 OwningCharacter가 캐스팅 실패 했습니다."));
	}
}

void UWorldAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	//소유한 캐릭터와 소유한 무브먼트 중 하나라도 nullptr이면
	if (!OwningCharacter || !OwingMovementComponent)
	{
		return;
	}
	
	//소유한 캐릭터의 속도
	FVector WorldVelocity = OwningCharacter->GetVelocity();						//월드 좌표 기준 속도
	FRotator CharacterRotation = OwningCharacter->GetActorRotation();			//월드 기준 회전값
	FVector LocalVelocity = CharacterRotation.UnrotateVector(WorldVelocity);	//회전된 정면을 기준으로 재계산
	LocalVelocity.Normalize();
	
	//실제 캐릭터의 정면에 대한 방향
	GroundSpeedForward = LocalVelocity.X;
	GroundSpeedRight = LocalVelocity.Y;
	
	//소유한 캐릭터 무브먼트의 움직임
	bHasAcceleration = OwingMovementComponent->GetCurrentAcceleration().SizeSquared2D() > 0.f;
}
