//Example Character to show Ragdolling and getting up
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "TPS_CPPCharacter.generated.h"


UCLASS(config=Game)
class ATPS_CPPCharacter : public ACharacter
{
	GENERATED_BODY()


public:
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* RagdollAction;

	/*-----------------------------------------------------------------------------------------------*/
	//Fall Ragdoll Variables

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
		FName RootBoneName;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
		FName PelvisBoneName;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
		FName NeckBoneName;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
		FName SnapshotPoseName;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
		FName PelvisSocketName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FallAnim")
		bool IsRagdoll;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FallAnim")
		bool bIsDown;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FallAnim")
		bool bIsFacingUp;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FallAnim")
		FVector MeshPelvisLoc;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FallAnim")
		UAnimMontage* FallUpAnimMontage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FallAnim")
		UAnimMontage* FallDownAnimMontage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FallAnim")
		UAnimMontage* GetupMontage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FallAnim")
		UAnimMontage* GetUpUpMontage;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FallAnim")
		UAnimMontage* GetUpDownMontage;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FallAnim")
		FTimerHandle SnapshotTimerHandle;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FallAnim")
		FTimerHandle CachePoseTimerHandle;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "CombatAnim")
		bool CanMove;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FVector CapsuleLoc;
	/*-----------------------------------------------------------------------------------------------*/

public:
	ATPS_CPPCharacter();
	virtual void Tick(float DeltaTime) override;

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:
	/*-----------------------------------------------------------------------------------------------*/
	//Fall Ragdoll functions
	UFUNCTION()
		void Ragdoll();

	/*Calculate if character fall facing up or down*/
	UFUNCTION(BlueprintCallable, BlueprintPure)
		bool AfterFallFacingDirection();

	/*Set up getting up vectors and rotations*/
	UFUNCTION(BlueprintCallable)
		void SetupGetupOrientation();

	/*Calculate pelvis location after ragdolling*/
	UFUNCTION(BlueprintCallable)
		void CalculatePelvisMeshLocation();

	/*Calls necessary functions to get up*/
	UFUNCTION(BlueprintCallable)
		void GettingUp();

	/*Ragdolls the character*/
	UFUNCTION(BlueprintCallable)
		void FallDown();

	/*In some instances of characters root bone might be added to physics simulation so our Ragdolling should use SetAllBodiesBelowSimulatePhysics*/
	UFUNCTION(BlueprintCallable)
		void FallDownKinematic();

	/*Caches Ragdol pose to transition between Cached pose and Get up animation*/
	UFUNCTION(BlueprintCallable)
		void CachePose();

	/*Calls snapshot function to send the pose into animation blueprint*/
	UFUNCTION(BlueprintCallable)
		void SnapshotPose();

	/*Selects the Get up animation if character is looking up or down in Ragdoll pose*/
	UFUNCTION(BlueprintCallable)
		void SetupGetupMontage();

	/*For smooth transition, Snapshotting pose need some frame delay. This function causes that */
	UFUNCTION(BlueprintCallable)
		void DelayAfterCachePose();

	/*This function releases player controller since ragdolling function locks players movement. Sets movement mode to Walking. 
	This function should call in an Animation Notify */
	UFUNCTION(BlueprintCallable)
		void EnableWalking();

	/*-----------------------------------------------------------------------------------------------*/

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
