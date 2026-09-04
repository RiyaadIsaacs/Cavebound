#include "CaveboundTurret.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

ACaveboundTurret::ACaveboundTurret()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// No default mesh
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	VisualMesh->SetCollisionObjectType(ECC_WorldDynamic);
	VisualMesh->SetCollisionResponseToAllChannels(ECR_Block);

	Health = MaxHealth;
}

void ACaveboundTurret::BeginPlay()
{
	Super::BeginPlay();

	// Pick up MaxHealth overrides from Blueprint defaults
	Health = MaxHealth;
}

void ACaveboundTurret::ApplyDamage(float Amount)
{
	if (IsDestroyed() || Amount <= 0.f)
	{
		return;
	}

	Health = FMath::Max(0.f, Health - Amount);
	PlayDamageFlash();

	if (IsDestroyed())
	{
		OnDestroyedByDamage();
	}
}

void ACaveboundTurret::PlayDamageFlash()
{
	TArray<UStaticMeshComponent*> Meshes;
	GetComponents<UStaticMeshComponent>(Meshes);
	FCaveboundDamageFlash::FlashMeshes(this, Meshes, HitFlashTimer, HitFlashColor, HitFlashDuration);
}

void ACaveboundTurret::OnDestroyedByDamage()
{
	Destroy();
}
