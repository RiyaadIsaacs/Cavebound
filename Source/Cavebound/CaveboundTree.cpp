#include "CaveboundTree.h"
#include "CaveboundGameMode.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundTree::ACaveboundTree()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TrunkMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrunkMesh"));
	TrunkMesh->SetupAttachment(SceneRoot);
	TrunkMesh->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	TrunkMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 3.0f));
	TrunkMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TrunkMesh->SetCollisionResponseToAllChannels(ECR_Block);

	CanopyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CanopyMesh"));
	CanopyMesh->SetupAttachment(SceneRoot);
	CanopyMesh->SetRelativeLocation(FVector(0.f, 0.f, 380.f));
	CanopyMesh->SetRelativeScale3D(FVector(3.2f, 3.2f, 2.4f));
	CanopyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CanopyMesh->SetCollisionResponseToAllChannels(ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		TrunkMesh->SetStaticMesh(CylinderMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		CanopyMesh->SetStaticMesh(SphereMesh.Object);
	}

	Health = MaxHealth;
}

void ACaveboundTree::ApplyDamage(float Amount)
{
	if (IsDestroyed())
	{
		return;
	}

	Health = FMath::Max(0.f, Health - Amount);
	if (IsDestroyed())
	{
		if (UWorld* World = GetWorld())
		{
			if (ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>())
			{
				GameMode->HandleTreeDestroyed();
			}
		}
	}
}
