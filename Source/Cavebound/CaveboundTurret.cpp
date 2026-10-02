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

	// Empty root so the mesh and the fire point can sit on the same actor.
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// Placeholder shape. A blueprint can swap the mesh later.
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	VisualMesh->SetCollisionObjectType(ECC_WorldDynamic);
	VisualMesh->SetCollisionResponseToAllChannels(ECR_Block);

	// Arrows are the normal shot unless a child changes this.
	ProjectileClass = ACaveboundArrow::StaticClass();

	Health = MaxHealth;
}

void ACaveboundTurret::BeginPlay()
{
	Super::BeginPlay();

	// Blueprint children can change MaxHealth. Start the bar at that number.
	Health = MaxHealth;
}

void ACaveboundTurret::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// A broken turret, or one that does not shoot, does nothing.
	if (!bAutoFire || IsDestroyed())
	{
		return;
	}

	// Only shoot while a combat round is running.
	if (UWorld* world = GetWorld())
	{
		if (const ACaveboundGameMode* gameMode = world->GetAuthGameMode<ACaveboundGameMode>())
		{
			if (gameMode->GetRoundState() != ECaveboundRoundState::Combat)
			{
				FireTime = 0.f;
				return;
			}
		}
	}

	// Wait until the fire timer is full, then shoot and start the wait again.
	FireTime += DeltaTime;
	if (FireTime >= FireInterval)
	{
		FireTime = 0.f;
		TryFireAtNearestEnemy();
	}
}

void ACaveboundTurret::ApplyDamage(float Amount)
{
	// Ignore hits on a dead turret, and ignore a hit of zero.
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
	// Flash every mesh on this turret red for a short time.
	TArray<UStaticMeshComponent*> meshList;
	GetComponents<UStaticMeshComponent>(meshList);
	FCaveboundDamageFlash::FlashMeshes(this, meshList, HitFlashTimer, HitFlashColor, HitFlashDuration);
}

void ACaveboundTurret::OnDestroyedByDamage()
{
	Destroy();
}

ACaveboundBaseEnemy* ACaveboundTurret::FindNearestEnemy() const
{
	UWorld* world = GetWorld();
	if (!world)
	{
		return nullptr;
	}

	// Look at every enemy and keep the closest one that is still inside range.
	TArray<AActor*> allEnemies;
	UGameplayStatics::GetAllActorsOfClass(world, ACaveboundBaseEnemy::StaticClass(), allEnemies);

	ACaveboundBaseEnemy* closestEnemy = nullptr;
	float closestDistance = AttackRange;
	const FVector turretLocation = GetActorLocation();

	for (AActor* foundActor : allEnemies)
	{
		ACaveboundBaseEnemy* enemy = Cast<ACaveboundBaseEnemy>(foundActor);
		if (!enemy || enemy->IsDead())
		{
			continue;
		}

		const float distanceToEnemy = FVector::Dist2D(turretLocation, enemy->GetActorLocation());
		if (distanceToEnemy <= closestDistance)
		{
			closestDistance = distanceToEnemy;
			closestEnemy = enemy;
		}
	}

	return closestEnemy;
}

void ACaveboundTurret::TryFireAtNearestEnemy()
{
	ACaveboundBaseEnemy* closestEnemy = FindNearestEnemy();
	UWorld* world = GetWorld();
	if (!closestEnemy || !world || !ProjectileClass)
	{
		return;
	}

	// Spawn the shot at the fire point and aim it at the enemy's middle.
	const FVector arrowSpawnPoint = GetActorTransform().TransformPosition(FirePointOffset);
	const FVector aimAtEnemy = closestEnemy->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FRotator aimDirection = (aimAtEnemy - arrowSpawnPoint).Rotation();

	FActorSpawnParameters spawnSettings;
	spawnSettings.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	spawnSettings.Owner = this;
	spawnSettings.Instigator = GetInstigator();

	// Spawn the projectile. A Cavebound arrow also gets the target, damage, and speed.
	if (AActor* spawnedArrow = world->SpawnActor<AActor>(ProjectileClass, arrowSpawnPoint, aimDirection, spawnSettings))
	{
		TArray<UPrimitiveComponent*> arrowParts;
		spawnedArrow->GetComponents<UPrimitiveComponent>(arrowParts);
		for (UPrimitiveComponent* arrowPart : arrowParts)
		{
			if (arrowPart)
			{
				arrowPart->MoveIgnoreActors.Add(this);
			}
		}

		if (ACaveboundArrow* arrowScript = Cast<ACaveboundArrow>(spawnedArrow))
		{
			arrowScript->Init(closestEnemy, ProjectileDamage, ProjectileSpeed);
		}
	}
}
