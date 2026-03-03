#include "World/WorldCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

AWorldCharacter::AWorldCharacter()
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
	// 기본적으로 카메라와 캐릭터 이동 별도로 구분, 캐릭터 Yaw 축 정면 방향만 카메라로 조작
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw   = false;
	bUseControllerRotationRoll  = false;
	
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate              = FRotator(0.f, 500.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed              = 400.f;
	GetCharacterMovement()->JumpZVelocity             = 700.f;
	GetCharacterMovement()->AirControl                = 0.35f;
}


void AWorldCharacter::BeginPlay()
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

void AWorldCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWorldCharacter::HandleMove);

		if (LookAction)
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AWorldCharacter::HandleLook);

		if (JumpAction)
		{
			EIC->BindAction(JumpAction, ETriggerEvent::Started,   this, &ACharacter::Jump);
			EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		
		if (FreeLookAction)
		{
			EIC->BindAction(FreeLookAction, ETriggerEvent::Started,   this, &AWorldCharacter::HandleFreeLookStart);
			EIC->BindAction(FreeLookAction, ETriggerEvent::Completed,   this, &AWorldCharacter::HandleFreeLookEnd);
		}
		
		if (RunAction)
		{
			EIC->BindAction(RunAction, ETriggerEvent::Started,   this, &AWorldCharacter::HandleRunToggle);
		}
	}
}

void AWorldCharacter::HandleMove(const FInputActionValue& Value)
{
	InputMoveAxis = Value.Get<FVector2D>();
	
	if (Controller == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Controller is nullptr"));
	}
	
	if (!Controller || InputMoveAxis.IsNearlyZero()) return;

	// 카메라 Yaw 기준으로 앞/오른쪽 방향 계산
	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector  Forward = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
	const FVector  Right   = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, InputMoveAxis.Y);
	AddMovementInput(Right,   InputMoveAxis.X);
}

void AWorldCharacter::HandleLook(const FInputActionValue& Value)
{
	InputLookAxis = Value.Get<FVector2D>();
	AddControllerYawInput(InputLookAxis.X);
	AddControllerPitchInput(-InputLookAxis.Y);
}

void AWorldCharacter::HandleFreeLookStart()
{
	//bUseControllerRotationYaw = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
}

void AWorldCharacter::HandleFreeLookEnd()
{
	//bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
}

void AWorldCharacter::HandleRunToggle()
{
	//달리기 토글
	bRunning == true ? bRunning = false : bRunning = true;
}
