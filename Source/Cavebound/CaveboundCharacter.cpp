#include "CaveboundCharacter.h"
#include "CaveboundGameMode.h"
#include "CaveboundTree.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace1D.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
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

	// Kept so older blueprints that still reference the placeholder do not fail to load.
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetVisibility(false);
	VisualMesh->SetHiddenInGame(true);

	// Barbarian is about 240cm tall, with feet at the mesh origin.
	GetCapsuleComponent()->SetCapsuleSize(42.f, 120.f);
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -120.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(
		TEXT("/Game/Characters/Barbarian/Barbarian.Barbarian"));
	if (MeshAsset.Succeeded())
	{
		BodyMesh = MeshAsset.Object;
		GetMesh()->SetSkeletalMeshAsset(BodyMesh);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(
		TEXT("/Game/Characters/Barbarian/M_BarbarianColor.M_BarbarianColor"));
	if (MaterialAsset.Succeeded())
	{
		BodyMaterial = MaterialAsset.Object;
		GetMesh()->SetMaterial(0, BodyMaterial);
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> IdleAsset(
		TEXT("/Game/Characters/Barbarian/Idle.Idle"));
	if (IdleAsset.Succeeded())
	{
		IdleAnim = IdleAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> WalkAsset(
		TEXT("/Game/Characters/Barbarian/Walk.Walk"));
	if (WalkAsset.Succeeded())
	{
		WalkAnim = WalkAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> RunAsset(
		TEXT("/Game/Characters/Barbarian/Run.Run"));
	if (RunAsset.Succeeded())
	{
		RunAnim = RunAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UBlendSpace> BlendAsset(
		TEXT("/Game/Characters/Barbarian/BS_PlayerMove.BS_PlayerMove"));
	if (BlendAsset.Succeeded())
	{
		LocomotionBlend = BlendAsset.Object;
	}
}

void ACaveboundCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (VisualMesh)
	{
		VisualMesh->SetStaticMesh(nullptr);
		VisualMesh->SetVisibility(false);
		VisualMesh->SetHiddenInGame(true);
	}

	USkeletalMeshComponent* Body = GetMesh();
	if (!Body)
	{
		return;
	}

	GetCapsuleComponent()->SetCapsuleSize(42.f, 120.f);
	Body->SetRelativeLocation(FVector(0.f, 0.f, -120.f));
	Body->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (BodyMesh)
	{
		Body->SetSkeletalMeshAsset(BodyMesh);
	}

	if (BodyMaterial)
	{
		const int32 SlotCount = FMath::Max(Body->GetNumMaterials(), 1);
		for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
		{
			Body->SetMaterial(SlotIndex, BodyMaterial);
		}
	}

	Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

#if WITH_EDITOR
	// The saved blend space has samples but no segment graph, which plays as a T-pose.
	if (LocomotionBlend)
	{
		LocomotionBlend->ResampleData();
		if (LocomotionBlend->GetBlendSpaceData().IsEmpty())
		{
			LocomotionBlend = nullptr;
		}
	}
#endif

	if (LocomotionBlend)
	{
		Body->PlayAnimation(LocomotionBlend, true);
	}
	UpdateLocomotion(0.f);
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

	// If can't collect wood, reset mining time and notify player
	if (UWorld* World = GetWorld())
	{
		if (ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>())
		{
			if (!GameMode->CanCollectWood())
			{
				MiningTime = 0.f;
				GameMode->NotifyMiningBlocked();
				return;
			}
		}
	}

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

	if (bHasMoveDestination)
	{
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
		}
		else
		{
			AddMovementInput(ToTarget.GetSafeNormal(), 1.f);
		}
	}

	UpdateLocomotion(DeltaTime);
}

void ACaveboundCharacter::UpdateLocomotion(float DeltaTime)
{
	USkeletalMeshComponent* Body = GetMesh();
	if (!Body)
	{
		return;
	}

	if (LocomotionBlend)
	{
		if (UAnimSingleNodeInstance* Node = Body->GetSingleNodeInstance())
		{
			const float TargetAxis = FMath::GetMappedRangeValueClamped(
				FVector2D(0.f, 600.f),
				FVector2D(0.f, 100.f),
				GetVelocity().Size2D());
			SmoothedLocomotion = FMath::FInterpTo(SmoothedLocomotion, TargetAxis, DeltaTime, LocomotionBlendSpeed);
			Node->SetBlendSpacePosition(FVector(SmoothedLocomotion, 0.f, 0.f));
		}
		return;
	}

	if (!IdleAnim)
	{
		return;
	}

	UAnimSequence* Desired = IdleAnim;
	const float Speed = GetVelocity().Size2D();
	if (Speed > 280.f && RunAnim)
	{
		Desired = RunAnim;
	}
	else if (Speed > 15.f && WalkAnim)
	{
		Desired = WalkAnim;
	}

	if (Desired == PlayingAnim)
	{
		return;
	}

	PlayingAnim = Desired;
	Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	Body->PlayAnimation(Desired, true);
}
