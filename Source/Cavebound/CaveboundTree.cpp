#include "CaveboundTree.h"
#include "CaveboundBaseEnemy.h"
#include "CaveboundGameMode.h"
#include "CaveboundPlayerController.h"
#include "CaveboundSoftVFX.h"
#include "CaveboundTreeProjectile.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundTree::ACaveboundTree()
{
	PrimaryActorTick.bCanEverTick = true;
	ProjectileClass = ACaveboundTreeProjectile::StaticClass();

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TrunkMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrunkMesh"));
	TrunkMesh->SetupAttachment(SceneRoot);
	TrunkMesh->SetRelativeLocation(FVector::ZeroVector);
	TrunkMesh->SetRelativeScale3D(FVector::OneVector);
	TrunkMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TrunkMesh->SetCollisionObjectType(ECC_WorldDynamic);
	TrunkMesh->SetCollisionResponseToAllChannels(ECR_Block);

	// Legacy slot — never show the old sphere/cylinder canopy.
	CanopyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CanopyMesh"));
	CanopyMesh->SetupAttachment(SceneRoot);
	CanopyMesh->SetStaticMesh(nullptr);
	CanopyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CanopyMesh->SetHiddenInGame(true);
	CanopyMesh->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> TreeMeshAsset(
		TEXT("/Game/Assets/Models/Tree/Tree.Tree"));
	if (TreeMeshAsset.Succeeded())
	{
		TrunkMesh->SetStaticMesh(TreeMeshAsset.Object);
		if (UBodySetup* BodySetup = TreeMeshAsset.Object->GetBodySetup())
		{
			// Avoid a huge auto-convex "cylinder" trap around the whole tree.
			BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
		}
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TreeMaterialAsset(
		TEXT("/Game/Assets/Models/Tree/M_Tree.M_Tree"));
	if (TreeMaterialAsset.Succeeded())
	{
		TrunkMesh->SetMaterial(0, TreeMaterialAsset.Object);
	}

	Health = MaxHealth;
}

void ACaveboundTree::BeginPlay()
{
	Super::BeginPlay();

	if (CanopyMesh)
	{
		CanopyMesh->SetStaticMesh(nullptr);
		CanopyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CanopyMesh->SetHiddenInGame(true);
		CanopyMesh->SetVisibility(false);
	}

	if (TrunkMesh)
	{
		if (UStaticMesh* Mesh = TrunkMesh->GetStaticMesh())
		{
			if (UBodySetup* BodySetup = Mesh->GetBodySetup())
			{
				BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
			}

			const FBoxSphereBounds Bounds = Mesh->GetBounds();
			const float BottomZ = Bounds.Origin.Z - Bounds.BoxExtent.Z;
			TrunkMesh->SetRelativeLocation(FVector(0.f, 0.f, -BottomZ));
		}

		if (UMaterialInterface* TreeMat = LoadObject<UMaterialInterface>(
				nullptr, TEXT("/Game/Assets/Models/Tree/M_Tree.M_Tree")))
		{
			const int32 NumMaterials = TrunkMesh->GetNumMaterials();
			for (int32 Index = 0; Index < FMath::Max(1, NumMaterials); ++Index)
			{
				TrunkMesh->SetMaterial(Index, TreeMat);
			}
		}

		TrunkMesh->RecreatePhysicsState();
	}

	const float EmberHeight = GetCanopyHeight() * 0.72f;
	if (!GreenEmbers)
	{
		GreenEmbers = ACaveboundHoverEmbers::SpawnAttached(
			this,
			FVector(0.f, 0.f, EmberHeight),
			FLinearColor(0.18f, 0.42f, 0.12f, 1.f));
	}
}

float ACaveboundTree::GetCanopyHeight() const
{
	if (TrunkMesh && TrunkMesh->GetStaticMesh())
	{
		const FBoxSphereBounds Bounds = TrunkMesh->Bounds;
		return FMath::Max(120.f, Bounds.Origin.Z + Bounds.BoxExtent.Z - GetActorLocation().Z);
	}
	return 420.f;
}

void ACaveboundTree::ApplyDamage(float Amount)
{
	if (Amount <= 0.f || Health <= 0.f)
	{
		return;
	}

	Health = FMath::Max(0.f, Health - Amount);
	PlayDamageFlash();

	if (ACaveboundPlayerController* PC = Cast<ACaveboundPlayerController>(
			UGameplayStatics::GetPlayerController(GetWorld(), 0)))
	{
		PC->PulseDamageVignette();
	}

	if (Health <= 0.f)
	{
		NotifyDestroyedByDamage();
	}
}

void ACaveboundTree::NotifyDestroyedByDamage()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>())
	{
		GameMode->HandleTreeDestroyed();
		return;
	}

	// Fallback if the world auth game mode cast fails for any reason.
	if (AGameModeBase* BaseMode = World->GetAuthGameMode())
	{
		if (ACaveboundGameMode* GameMode = Cast<ACaveboundGameMode>(BaseMode))
		{
			GameMode->HandleTreeDestroyed();
		}
	}
}

float ACaveboundTree::Heal(float Amount)
{
	if (IsDestroyed() || Amount <= 0.f || MaxHealth <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}

	const float Before = Health;
	Health = FMath::Min(MaxHealth, Health + Amount);
	return Health - Before;
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

	FCaveboundDamageFlash::FlashMeshes(this, Meshes, HitFlashTimer, HitFlashColor, HitFlashDuration);
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

	const FVector SpawnLocation = GetActorLocation() + FVector(0.f, 0.f, GetCanopyHeight() * 0.85f);
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;

	if (ACaveboundTreeProjectile* Projectile = World->SpawnActor<ACaveboundTreeProjectile>(ProjectileClass,SpawnLocation,FRotator::ZeroRotator,SpawnParams))
	{
		Projectile->Init(Target, ProjectileDamage, ProjectileSpeed);
	}
}
