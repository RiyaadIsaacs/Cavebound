#include "CaveboundTurret.h"
#include "CaveboundArrow.h"
#include "CaveboundBaseEnemy.h"
#include "CaveboundGameMode.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

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

	ProjectileClass = ACaveboundArrow::StaticClass();

	Health = MaxHealth;
}

void ACaveboundTurret::BeginPlay()
{
	Super::BeginPlay();

	// Pick up MaxHealth overrides from Blueprint defaults
	Health = MaxHealth;
}

void ACaveboundTurret::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bAutoFire || IsDestroyed())
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

ACaveboundBaseEnemy* ACaveboundTurret::FindNearestEnemy() const
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
	const FVector Origin = GetActorLocation();

	for (AActor* Actor : FoundEnemies)
	{
		ACaveboundBaseEnemy* Enemy = Cast<ACaveboundBaseEnemy>(Actor);
		if (!Enemy || Enemy->IsDead())
		{
			continue;
		}

		const float Distance = FVector::Dist2D(Origin, Enemy->GetActorLocation());
		if (Distance <= BestDistance)
		{
			BestDistance = Distance;
			Nearest = Enemy;
		}
	}

	return Nearest;
}

void ACaveboundTurret::TryFireAtNearestEnemy()
{
	ACaveboundBaseEnemy* Target = FindNearestEnemy();
	UWorld* World = GetWorld();
	if (!Target || !World || !ProjectileClass)
	{
		return;
	}

	const FVector SpawnLocation = GetActorTransform().TransformPosition(FirePointOffset);
	const FVector AimPoint = Target->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FRotator AimRotation = (AimPoint - SpawnLocation).Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();

	// BP_Arrow / CaveboundArrow: aim at target; C++ arrows also get Init for homing + damage
	if (AActor* Spawned = World->SpawnActor<AActor>(ProjectileClass, SpawnLocation, AimRotation, SpawnParams))
	{
		TArray<UPrimitiveComponent*> Primitives;
		Spawned->GetComponents<UPrimitiveComponent>(Primitives);
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			if (Primitive)
			{
				Primitive->MoveIgnoreActors.Add(this);
			}
		}

		if (ACaveboundArrow* Arrow = Cast<ACaveboundArrow>(Spawned))
		{
			Arrow->Init(Target, ProjectileDamage, ProjectileSpeed);
		}
	}
}
