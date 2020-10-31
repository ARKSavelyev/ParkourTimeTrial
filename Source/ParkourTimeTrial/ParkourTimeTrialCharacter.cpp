// Copyright Epic Games, Inc. All Rights Reserved.

#include "ParkourTimeTrialCharacter.h"
#include "ParkourTimeTrialProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "Runtime/Engine/Classes/GameFramework/CharacterMovementComponent.h"
#include "Runtime/Engine/Public/TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AParkourTimeTrialCharacter

AParkourTimeTrialCharacter::AParkourTimeTrialCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f; 

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	// Uncomment the following line to turn motion controllers on by default:
	//bUsingMotionControllers = true;
	JumpHeight = 600.f;
	RegularAirControl = 0.5f;
	GetCharacterMovement()->AirControl = RegularAirControl;
	GetCharacterMovement()->MaxWalkSpeed = 750.0f;
	//GetCharacterMovement()->MaxAcceleration = 800.0f;
	CanDash = true;
	DashDistance = 6000.f;
	DashCooldown = 2.f;
	DashStop = 0.1f;
	MultiJumpMaximum = 2;
	WallRunAirControl = 1.f;
	WallRunJumpLaunchMultiplier = 500;
	WallRunTilt = 15.f;
	WallRunTiltRate = 0.2f;
}

void AParkourTimeTrialCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));
	Mesh1P->SetHiddenInGame(false, true);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AParkourTimeTrialCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AParkourTimeTrialCharacter::DoubleJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	//Bind Dash Events
	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &AParkourTimeTrialCharacter::Dash);
	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AParkourTimeTrialCharacter::OnFire);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AParkourTimeTrialCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AParkourTimeTrialCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AParkourTimeTrialCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AParkourTimeTrialCharacter::LookUpAtRate);
}

void AParkourTimeTrialCharacter::OnFire()
{
	// try and fire a projectile
	if (ProjectileClass != NULL)
	{
		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			const FRotator SpawnRotation = GetControlRotation();
			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);

			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

			// spawn the projectile at the muzzle
			World->SpawnActor<AParkourTimeTrialProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
		}
	}

	// try and play the sound if specified
	if (FireSound != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if (FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void AParkourTimeTrialCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AParkourTimeTrialCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AParkourTimeTrialCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AParkourTimeTrialCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AParkourTimeTrialCharacter::Landed(const FHitResult& Hit)
{
	MultiJumpCounter = 0;
}

void AParkourTimeTrialCharacter::DoubleJump()
{
	if (MultiJumpCounter <= MultiJumpMaximum)
	{
		FVector crossDirection = FVector(0,0,0);
		if (IsWallRunning)
		{
			EndWallRun(EWallRunEndCause::Jump);
			FVector Z;
			if (WallRunSide == EWallRunSide::Right)
			{
				Z = FVector(0,0,-1);
			}
			else
			{
				Z = FVector(0, 0, 1);
			}
			crossDirection = FVector::CrossProduct(WallRunDirection, Z);
			crossDirection = crossDirection * WallRunJumpLaunchMultiplier;
		}
		LaunchCharacter(FVector(crossDirection.X, crossDirection.Y, JumpHeight), false, true);
		MultiJumpCounter++;
	}
}

void AParkourTimeTrialCharacter::Dash()
{
	if (CanDash)
	{
		GetCharacterMovement()->BrakingFrictionFactor = 0.f;
		LaunchCharacter(GetDirectionForDash() * DashDistance, true, true);
		CanDash = false;
		GetWorldTimerManager().SetTimer(DashCountdownHandle, this, &AParkourTimeTrialCharacter::StopDashing, DashStop, false);
	}
}

void AParkourTimeTrialCharacter::StopDashing()
{
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->BrakingFrictionFactor = 2.f;
	GetWorldTimerManager().SetTimer(DashCountdownHandle, this, &AParkourTimeTrialCharacter::ResetDash, DashCooldown, false);
}

void AParkourTimeTrialCharacter::ResetDash()
{
	CanDash = true;
}

FVector AParkourTimeTrialCharacter::GetDirectionForDash()
{
	auto Velocity = GetCharacterMovement()->GetLastInputVector();
	if (Velocity.IsNearlyZero())
		return FVector(FirstPersonCameraComponent->GetForwardVector().X, FirstPersonCameraComponent->GetForwardVector().Y, 0).GetSafeNormal();
	Velocity.Z = 0;
	return Velocity.GetSafeNormal();
}

bool AParkourTimeTrialCharacter::IsSurfaceValidForWallRun(FVector surfaceNormal)
{
	auto z = surfaceNormal.Z;
	if (z < -0.05)
		return false;
	auto inclineCheckVector =  FVector(surfaceNormal.X, surfaceNormal.Y, 0);
	inclineCheckVector.Normalize();
	float dotProduct = FVector::DotProduct(inclineCheckVector, surfaceNormal);
	float rads = FMath::Acos(dotProduct);
	float angle = FMath::RadiansToDegrees(rads);
	auto walkableFloor = GetCharacterMovement()->GetWalkableFloorAngle();
	return angle < walkableFloor;
}

void AParkourTimeTrialCharacter::GetWallRunSideAndDirection(FVector surfaceNormal, FVector& Direction, EWallRunSide& Side)
{
	auto actorRightVector = GetActorRightVector();
	auto surfaceNormal2D = FVector2D(surfaceNormal); 
	auto actorRightVector2D = FVector2D(actorRightVector);
	auto dotProduct = FVector2D::DotProduct(actorRightVector2D, surfaceNormal2D);
	FVector Z;
	if (dotProduct>0)
	{
		Side = EWallRunSide::Right;
		Z = FVector(0,0,1);
	}
	else
	{
		Side = EWallRunSide::Left;
		Z = FVector(0, 0, -1);
	}
	Direction = FVector::CrossProduct(surfaceNormal, Z);
}


bool AParkourTimeTrialCharacter::CheckKeysAreDown(EWallRunSide Side)
{
	auto forwardAxis = GetInputAxisValue("MoveForward");
	auto rightAxis = GetInputAxisValue("MoveRight");
	if (forwardAxis > 0.1)
	{
		if (Side == EWallRunSide::Right && rightAxis < -0.1)
			return true;

		if (Side == EWallRunSide::Left && rightAxis > 0.1)
			return true;
	}
	return false;
}

void AParkourTimeTrialCharacter::PerformRotation()
{
	auto lerpResult = FMath::Lerp(CurrentRotation, WallRunTargetRotation, 0.05f);
	CurrentRotation = lerpResult;
	auto playerRotation = GetController()->GetControlRotation();
	playerRotation.Roll = lerpResult;
	GetController()->SetControlRotation(playerRotation);
}

void AParkourTimeTrialCharacter::RotateCharacter()
{
	if (WallRunSide == EWallRunSide::Left)
	{
		if ((IsWallRunning && CurrentRotation > WallRunTargetRotation) || (!IsWallRunning && CurrentRotation<WallRunTargetRotation))
		{
			PerformRotation();
		}
	}
	else if (WallRunSide == EWallRunSide::Right)
	{
		if ((IsWallRunning && CurrentRotation < WallRunTargetRotation) || (!IsWallRunning && CurrentRotation > WallRunTargetRotation))
		{
			PerformRotation();
		}
	}
	else
	{
		GetWorld()->GetTimerManager().ClearTimer(CameraTiltHandle);
	}
}

void AParkourTimeTrialCharacter::StartCameraRotation()
{
	const auto controller = GetController();
	WallRunBeginRotation = controller->GetControlRotation().Roll;
	CurrentRotation = WallRunBeginRotation;
	int Multiplier;
	if (WallRunSide == EWallRunSide::Right)
	{
		Multiplier = 1;
	}
	else
	{
		Multiplier = -1;
	}
	WallRunTargetRotation = CurrentRotation + Multiplier * WallRunTilt;
	GetWorldTimerManager().SetTimer(CameraTiltHandle, this, &AParkourTimeTrialCharacter::RotateCharacter, 0.01f, true);
}

void AParkourTimeTrialCharacter::ReverseCameraRotation()
{
	CurrentRotation = WallRunTargetRotation;
	WallRunTargetRotation = WallRunBeginRotation;
	GetWorldTimerManager().SetTimer(CameraTiltHandle, this, &AParkourTimeTrialCharacter::RotateCharacter, 0.01f, true);
}

void AParkourTimeTrialCharacter::BeginWallRun()
{
	GetCharacterMovement()->AirControl = WallRunAirControl;
	MultiJumpCounter = 0;
	GetCharacterMovement()->GravityScale = 0;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0, 0, 1));//SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Z);
	IsWallRunning = true;
	StartCameraRotation();
}

void AParkourTimeTrialCharacter::EndWallRun(EWallRunEndCause endCause)
{
	GetCharacterMovement()->AirControl = RegularAirControl;
	GetCharacterMovement()->GravityScale = 1;
	GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0, 0, 0));
	if (endCause == EWallRunEndCause::Fall)
	{
		MultiJumpCounter++;
	}
	IsWallRunning = false;
	ReverseCameraRotation();
}