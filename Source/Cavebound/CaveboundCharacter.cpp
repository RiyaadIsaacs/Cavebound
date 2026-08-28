#include "CaveboundCharacter.h"
#include "CaveboundGameMode.h"
#include "CaveboundTree.h"
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
	MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
	MoveComp->MaxWalkSpeed = 600.f;
	MoveComp->AirControl = 1.f;

	// Camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1200.f;
	CameraBoom->SetRelativeRotation(FRotator(-55.f, 0.f, 0.f)); // pitch (-55) for almost top-down view
	CameraBoom->bDoCollisionTest = false; // do not pull in if a wall is in the way
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

// Rotate the camera boom based on mouse movement while right clicking
void ACaveboundCharacter::OrbitCamera(float MouseX, float MouseY)
{
	if (!CameraBoom)
	{
		return;
	}

	FRotator Rotation = CameraBoom->GetRelativeRotation();
	Rotation.Yaw += MouseX * CameraOrbitSensitivity;
	// Mouse up (positive Y) looks more top-down
	Rotation.Pitch = FMath::Clamp(
		Rotation.Pitch - MouseY * CameraOrbitSensitivity,
		CameraMinPitch,
		CameraMaxPitch);
	Rotation.Roll = 0.f;
	CameraBoom->SetRelativeRotation(Rotation);
}

void ACaveboundCharacter::SetMoveDestination(const FVector& Destination)
{
	StopMining();
	MoveDestination = Destination;
	bHasMoveDestination = true;
}

void ACaveboundCharacter::SetTreeTarget(ACaveboundTree* Tree)
{
	if (!Tree)
	{
		SetMoveDestination(GetActorLocation());
		return;
	}

	TargetTree = Tree;
	MiningTime = 0.f;

	// Walk toward the trunk, not the click point on the canopy (it can be in the air).
	MoveDestination = Tree->GetActorLocation();
	bHasMoveDestination = true;
}

void ACaveboundCharacter::StopMining()
{
	TargetTree = nullptr;
	MiningTime = 0.f;
}

void ACaveboundCharacter::TryMine(float DeltaTime)
{
	ACaveboundTree* Tree = TargetTree.Get();
	if (!Tree || Tree->IsDestroyed())
	{
		StopMining();
		return;
	}

	FVector ToTree = Tree->GetActorLocation() - GetActorLocation();
	ToTree.Z = 0.f;
	if (ToTree.Size() > Tree->GetMineRange())
	{
		MiningTime = 0.f;
		return;
	}

	bHasMoveDestination = false;
	MiningTime += DeltaTime;

	if (MiningTime >= Tree->GetHarvestInterval())
	{
		MiningTime = 0.f;
		if (UWorld* World = GetWorld())
		{
			if (ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>())
			{
				GameMode->AddWood(Tree->GetWoodPerHarvest());
			}
		}
	}
}

void ACaveboundCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (TargetTree.IsValid())
	{
		TryMine(DeltaTime);
	}

	if (!bHasMoveDestination)
	{
		return;
	}

	const FVector Current = GetActorLocation();
	FVector ToTarget = MoveDestination - Current;
	ToTarget.Z = 0.f;

	// When walking to the tree, stop at mine range so we do not walk into the trunk.
	float StopDistance = ArrivalDistance;
	if (ACaveboundTree* Tree = TargetTree.Get())
	{
		StopDistance = Tree->GetMineRange();
	}

	if (ToTarget.Size() <= StopDistance)
	{
		bHasMoveDestination = false;
		return;
	}

	AddMovementInput(ToTarget.GetSafeNormal(), 1.f);
}
