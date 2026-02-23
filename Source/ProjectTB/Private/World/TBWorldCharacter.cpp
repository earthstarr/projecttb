#include "World/TBWorldCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

ATBWorldCharacter::ATBWorldCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// ─── 스프링암 & 카메라 ────────────────────────────────────────────────────
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength         = 700.f;
	SpringArm->bUsePawnControlRotation = true;   // 마우스로 카메라 회전
	SpringArm->bInheritPitch           = true;
	SpringArm->bInheritYaw             = true;
	SpringArm->bInheritRoll            = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	// ─── 이동 세팅 ───────────────────────────────────────────────────────────
	// 카메라 방향 기준으로 이동, 캐릭터가 이동 방향 바라보게
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate              = FRotator(0.f, 500.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed              = 500.f;
	GetCharacterMovement()->JumpZVelocity             = 700.f;
	GetCharacterMovement()->AirControl                = 0.35f;
}

void ATBWorldCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Enhanced Input 컨텍스트 등록
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (WorldMappingContext)
				Subsystem->AddMappingContext(WorldMappingContext, 0);
		}
	}
}

void ATBWorldCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATBWorldCharacter::HandleMove);

		if (LookAction)
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATBWorldCharacter::HandleLook);

		if (JumpAction)
		{
			EIC->BindAction(JumpAction, ETriggerEvent::Started,   this, &ACharacter::Jump);
			EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
	}
}

void ATBWorldCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	if (!Controller || Axis.IsNearlyZero()) return;

	// 카메라 Yaw 기준으로 앞/오른쪽 방향 계산
	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector  Forward = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
	const FVector  Right   = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, Axis.Y);
	AddMovementInput(Right,   Axis.X);
}

void ATBWorldCharacter::HandleLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(-Axis.Y);
}
