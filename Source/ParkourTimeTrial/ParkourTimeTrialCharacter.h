// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ParkourTimeTrialCharacter.generated.h"

class UInputComponent;

UENUM(BlueprintType)
enum class EWallRunSide : uint8 {
	Left       UMETA(DisplayName = "Left"),
	Right      UMETA(DisplayName = "Right"),	
};

UENUM(BlueprintType)
enum class EWallRunEndCause : uint8 {
	Fall       UMETA(DisplayName = "Fell Off"),
	Jump      UMETA(DisplayName = "Jumped Off"),
};

UCLASS(config=Game)
class AParkourTimeTrialCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	class USkeletalMeshComponent* Mesh1P;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* FP_Gun;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USceneComponent* FP_MuzzleLocation;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCameraComponent;
public:
	AParkourTimeTrialCharacter();

	/** Returns Mesh1P subobject **/
	FORCEINLINE class USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	UFUNCTION(BlueprintCallable)
		void DoubleJump();

	UPROPERTY(EditAnywhere)
		int MultiJumpCounter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int MultiJumpMaximum;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float JumpHeight;

	UFUNCTION(BlueprintCallable)
		void Dash();

	UFUNCTION(BlueprintCallable)
		void StopDashing();

	UFUNCTION(BlueprintCallable)
		void ResetDash();

	UPROPERTY()
		FTimerHandle UnusedHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float DashDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float DashCooldown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool CanDash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float DashStop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool IsWallRunning;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float RegularAirControl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float WallRunAirControl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int WallRunJumpLaunchMultiplier;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector WallRunDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		EWallRunSide WallRunSide;
	
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseLookUpRate;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
		FVector GunOffset;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
		TSubclassOf<class AParkourTimeTrialProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
		class USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
		class UAnimMontage* FireAnimation;

	/** Whether to use motion controller location for aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
		uint32 bUsingMotionControllers : 1;

protected:
	virtual void BeginPlay();

	void Landed(const FHitResult& Hit) override;
	
	/** Fires a projectile. */
	void OnFire();

	/** Resets HMD orientation and position in VR. */
	void OnResetVR();

	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

	UFUNCTION()
		FVector GetDirectionForDash();

	UFUNCTION(BlueprintCallable)
		bool IsSurfaceValidForWallRun(FVector surfaceNormal);

	UFUNCTION(BlueprintCallable)
		void GetWallRunSideAndDirection(FVector surfaceNormal, FVector& Direction, EWallRunSide& Side);

	UFUNCTION(BlueprintCallable)
		void BeginWallRun();

	UFUNCTION(BlueprintCallable)
		void EndWallRun(EWallRunEndCause endCause);

	UFUNCTION(BlueprintCallable)
		bool CheckKeysAreDown(EWallRunSide Side);
};