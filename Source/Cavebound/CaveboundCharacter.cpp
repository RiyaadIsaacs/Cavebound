#include "CaveboundCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundCharacter::ACaveboundCharacter()
{

	PrimaryActorTick.bCanEverTick = true;

	// Mouse look does not rotate the body.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f); // Yaw degrees per second
	MoveComp->MaxWalkSpeed = 600.f;                      // Around 6 metres per second
	MoveComp->AirControl = 1.f;                          // still steer if a click happens while landing

	// Camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);          // follow the capsule
	CameraBoom->TargetArmLength = 1200.f;                // camera distance. raise to see more map
	CameraBoom->SetRelativeRotation(FRotator(-55.f, 0.f, 0.f)); // pitch (-55) for almost top-down view
	CameraBoom->bDoCollisionTest = false;                // do not pull in if a wall is in the way
	CameraBoom->bUsePawnControlRotation = false;
	// Keep the camera locked while the body turns to walk.
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	// Actual camera on the end of the boom 
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Placeholder body (engine cube)
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Basic cube sizing through code, pretty cool
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMesh.Object);
		VisualMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.8f)); // Actually set the cube mesh's size
	}
}

void ACaveboundCharacter::SetMoveDestination(const FVector& Destination)
{
	MoveDestination = Destination;
	bHasMoveDestination = true;
}

void ACaveboundCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bHasMoveDestination)
	{
		return;
	}

	const FVector Current = GetActorLocation();
	FVector ToTarget = MoveDestination - Current;
	ToTarget.Z = 0.f; // remove z axis, don't want to fly to the click height

	// Stop distance in cm (Unreal units)
	if (ToTarget.Size() <= ArrivalDistance)
	{
		bHasMoveDestination = false;
		return;
	}

	AddMovementInput(ToTarget.GetSafeNormal(), 1.f);
}
