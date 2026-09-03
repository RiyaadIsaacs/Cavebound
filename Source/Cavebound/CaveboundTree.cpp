#include "CaveboundTree.h"
#include "CaveboundBaseEnemy.h"
#include "CaveboundGameMode.h"
#include "CaveboundTreeProjectile.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundTree::ACaveboundTree()
{
	PrimaryActorTick.bCanEverTick = true;
	ProjectileClass = ACaveboundTreeProjectile::StaticClass();

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
	PlayDamageFlash();

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

void ACaveboundTree::PlayDamageFlash()
{
	TArray<UStaticMeshComponent*> Meshes;
	if (TrunkMesh)
	{
		Meshes.Add(TrunkMesh);
	}
	if (CanopyMesh)
	{
		Meshes.Add(CanopyMesh);
	}

	FCaveboundDamageFlash::FlashMeshes(this,Meshes,HitFlashTimer,HitFlashCache,HitFlashColor,HitFlashDuration);
}

void ACaveboundTree::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsDestroyed())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (const ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>())
		{
			if (GameMode->GetRoundState() != ECaveboundRoundState::Combat)
			{
				FireTime = 0.f;
				return;
			}
		}
	}

	FireTime += DeltaTime;
	if (FireTime >= FireInterval)
	{
		FireTime = 0.f;
		TryFireAtNearestEnemy();
	}
}

ACaveboundBaseEnemy* ACaveboundTree::FindNearestEnemy() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsOfClass(World, ACaveboundBaseEnemy::StaticClass(), FoundEnemies);

	ACaveboundBaseEnemy* Nearest = nullptr;
	float BestDistance = AttackRange;
	const FVector TreeLocation = GetActorLocation();

	// Find the nearest enemy within attack range
	for (AActor* Actor : FoundEnemies)
	{
		ACaveboundBaseEnemy* Enemy = Cast<ACaveboundBaseEnemy>(Actor);
		if (!Enemy || Enemy->IsDead())
		{
			continue;
		}

		const float Distance = FVector::Dist2D(TreeLocation, Enemy->GetActorLocation());
		if (Distance <= BestDistance)
		{
			BestDistance = Distance;
			Nearest = Enemy;
		}
	}

	return Nearest;
}

void ACaveboundTree::TryFireAtNearestEnemy()
{
	ACaveboundBaseEnemy* Target = FindNearestEnemy();
	UWorld* World = GetWorld();
	if (!Target || !World || !ProjectileClass)
	{
		return;
	}

	const FVector SpawnLocation = GetActorLocation() + FVector(0.f, 0.f, 420.f);
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	if (ACaveboundTreeProjectile* Projectile = World->SpawnActor<ACaveboundTreeProjectile>(ProjectileClass,SpawnLocation,FRotator::ZeroRotator,SpawnParams))
	{
		Projectile->Init(Target, ProjectileDamage, ProjectileSpeed);
	}
}
