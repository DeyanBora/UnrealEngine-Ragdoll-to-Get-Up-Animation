#include "TPS_CPPCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "Engine/LatentActionManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"


#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "PhysicsEngine/ConstraintInstance.h"
//////////////////////////////////////////////////////////////////////////
// ATPS_CPPCharacter

ATPS_CPPCharacter::ATPS_CPPCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	RootBoneName = FName(TEXT("root"));
	PelvisBoneName = FName(TEXT("pelvis"));
	NeckBoneName = FName(TEXT("neck_01"));

	SnapshotPoseName = FName(TEXT("finalpose"));

	IsRagdoll = false;
	bIsDown = false;


}

void ATPS_CPPCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ATPS_CPPCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	
	if (bIsDown)
	{
		CalculatePelvisMeshLocation();
		GetCapsuleComponent()->SetWorldLocation(MeshPelvisLoc, false);
	}
	else
	{
		MeshPelvisLoc = GetMesh()->GetSocketLocation(PelvisBoneName)
			+ FVector(0.0, 0.0, 89.0);//for UE mannequin
	}
}


//////////////////////////////////////////////////////////////////////////
// Input

void ATPS_CPPCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) 
  {
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATPS_CPPCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATPS_CPPCharacter::Look);
		EnhancedInputComponent->BindAction(RagdollAction, ETriggerEvent::Triggered, this, &ATPS_CPPCharacter::Ragdoll);
	}

}

void ATPS_CPPCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ATPS_CPPCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

/*-----------------------------------------------------------------------------------------------*/
//Get Up / Ragdoll FUNCTIONS

void ATPS_CPPCharacter::Ragdoll()
{
	UE_LOG(LogTemp, Warning, TEXT("Ragdoll function"));
	
	if (IsRagdoll)
	{
		GettingUp();
		IsRagdoll = false;
		CameraBoom->bEnableCameraLag = false;
	}
	else
	{
		CameraBoom->bEnableCameraLag = true;
		FallDown();
		IsRagdoll = true;
	}	
}

bool ATPS_CPPCharacter::AfterFallFacingDirection()
{
	bool bIsFacingUpF = false;

	//If Pelvis Bone has weird rotation use Pelvis Socket
	FRotator PelvisBoneRotation = GetMesh()->GetSocketRotation(PelvisBoneName);
	FVector RightVector = UKismetMathLibrary::GetRightVector(PelvisBoneRotation);
	double DotProduct = FVector::DotProduct(RightVector, FVector(0.0, 0.0, 1.0));

	if (DotProduct >= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Facing up"));
		bIsFacingUpF = true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Facing down"));
		bIsFacingUpF = false;
	}

	return bIsFacingUpF;
}

void ATPS_CPPCharacter::SetupGetupOrientation()
{
	FVector NeckSocketLocation = GetMesh()->GetSocketLocation(NeckBoneName);
	FVector PelvisSocketLocation = GetMesh()->GetSocketLocation(PelvisBoneName);
	FVector FinalVector = FVector(0.0, 0.0, 0.0);
  
	if (bIsFacingUp)
	{
		FinalVector = PelvisSocketLocation - NeckSocketLocation;
	}
	else
	{
		FinalVector = NeckSocketLocation - PelvisSocketLocation;
	}

	FRotator ActorRot = UKismetMathLibrary::MakeRotFromZX(FVector(0.0, 0.0, 1.0), FinalVector);
	FTransform PlayerTransform = FTransform(ActorRot, MeshPelvisLoc, FVector(1.0, 1.0, 1.0));
	SetActorTransform(PlayerTransform, false);
}

void ATPS_CPPCharacter::SetupGetupMontage()
{
	if (bIsFacingUp)
	{
		GetupMontage = GetUpUpMontage;
	}
	else
	{
		GetupMontage = GetUpDownMontage;
	}
}

void ATPS_CPPCharacter::CalculatePelvisMeshLocation()
{
	FVector SocLoc = GetMesh()->GetSocketLocation(PelvisBoneName);
	FVector FinalLoc = SocLoc + (FVector(0.0, 0.0, -1.0) * 100);
	FHitResult HitRes = FHitResult();
	FCollisionQueryParams TraceParams;
	TraceParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(HitRes, SocLoc, FinalLoc, ECC_Visibility, TraceParams);

	if (HitRes.bBlockingHit)
	{
		CapsuleLoc = HitRes.Location 
			+ FVector(0.0, 0.0, 89.0); //for UE mannequin
	}
	else
	{
		CapsuleLoc = GetMesh()->GetSocketLocation(PelvisBoneName)
			+ FVector(0.0, 0.0, 89.0);// for UE mannequin
	}

	MeshPelvisLoc = UKismetMathLibrary::VInterpTo(MeshPelvisLoc, CapsuleLoc, UGameplayStatics::GetWorldDeltaSeconds(GetWorld()), 10.0);
}

//Get up if down
void ATPS_CPPCharacter::GettingUp()
{
	UE_LOG(LogTemp, Warning, TEXT("Get Up"));
	CachePose();

	GetWorld()->GetTimerManager().SetTimer(CachePoseTimerHandle, this, &ATPS_CPPCharacter::DelayAfterCachePose, 0.2f, false);

}

//Called from Getting Up
void ATPS_CPPCharacter::DelayAfterCachePose()
{
	UE_LOG(LogTemp, Warning, TEXT("DelayAfterCachePose"));
	
	
	GetMesh()->SetSimulatePhysics(false);

	bIsDown = false;
	CanMove = true;
	
	float a = GetMesh()->GetAnimInstance()->Montage_Play(GetupMontage, 1.0f);
}

//Fall Function
void ATPS_CPPCharacter::FallDown()
{
	UE_LOG(LogTemp, Warning, TEXT("Fall Down"));
	GetMesh()->SetSimulatePhysics(true);

	bIsDown = true;
	CanMove = false;

	GetCharacterMovement()->SetMovementMode(MOVE_None);
}

void ATPS_CPPCharacter::FallDownKinematic()
{
	GetMesh()->SetAllBodiesBelowSimulatePhysics(PelvisBoneName, true);

	bIsDown = true;
	CanMove = false;

	GetCharacterMovement()->SetMovementMode(MOVE_None);
}

//Cache and Calculate Ragdoll Pose
void ATPS_CPPCharacter::CachePose()
{
	//Calculate Facing Direction
	bIsFacingUp = AfterFallFacingDirection();

	//Calculate vector for forward direction and Get up location vector
	SetupGetupOrientation();

	//Set montage by looking facing up or down
	SetupGetupMontage();

	//Actually the example has two frame delays so we tried two one frame delay here
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ATPS_CPPCharacter::SnapshotPose);
}

//Fired from CachePose - Takes Snapshot of Grounded pose. We will use this for Blending between Grounded pose to standing up Animation Start.
void ATPS_CPPCharacter::SnapshotPose()
{
	//This function will cause 2 Frame delays for Snapshotting pose 1 from before and one from this.
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([this] {UE_LOG(LogTemp, Warning, TEXT("SnapshotPose")); GetMesh()->GetAnimInstance()->SavePoseSnapshot(SnapshotPoseName); }));
}

//Player can walk after getting up
void ATPS_CPPCharacter::EnableWalking()
{
	GetCharacterMovement()->SetMovementMode(EMovementMode(MOVE_Walking), 0);
	GetMesh()->GetAnimInstance()->Montage_Stop(0.25f);
}
