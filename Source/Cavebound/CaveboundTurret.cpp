#include "CaveboundTurret.h"
#include "CaveboundArrow.h"
#include "CaveboundBaseEnemy.h"
#include "CaveboundGameMode.h"
#include "Components/ArrowComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundTurret::ACaveboundTurret()
{
	PrimaryActorTick.bCanEverTick = true;

	// Empty root so the mesh and the fire point can sit on the same actor.
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// The archer tower is the normal turret body. The file is about 15 meters tall, so it is scaled down.
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetRelativeScale3D(FVector(0.2f));
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	VisualMesh->SetCollisionObjectType(ECC_WorldDynamic);
	VisualMesh->SetCollisionResponseToAllChannels(ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> towerMesh(
		TEXT("/Game/Assets/Models/Towers/SM_ArcherTower_LVL1.SM_ArcherTower_LVL1"));
	if (towerMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(towerMesh.Object);
	}

	// Shoot from the top of the tower, toward its front.
	FirePointOffset = FVector(40.f, 0.f, 220.f);

	// Arrows are the normal shot unless a child changes this.
	ProjectileClass = ACaveboundArrow::StaticClass();

	Health = MaxHealth;
}

void ACaveboundTurret::BeginPlay()
{
	Super::BeginPlay();

	// Blueprint children can change MaxHealth. Start the bar at that number.
	Health = MaxHealth;

	// The fire-point arrow is added on the blueprint, so it exists once play starts.
	RememberFirePoint();
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

	// Spin toward the closest enemy, then shoot only while it sits in front of the fire point.
	ACaveboundBaseEnemy* closestEnemy = FindNearestEnemy();
	TurnTowardEnemy(closestEnemy, DeltaTime);

	FireTime += DeltaTime;
	if (FireTime >= FireInterval && TryFireAtEnemy(closestEnemy))
	{
		FireTime = 0.f;
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

void ACaveboundTurret::RememberFirePoint()
{
	// Prefer an arrow whose name says fire. Fall back to the first arrow on the actor.
	TArray<UArrowComponent*> arrows;
	GetComponents<UArrowComponent>(arrows);

	FirePoint = nullptr;
	for (UArrowComponent* arrow : arrows)
	{
		if (!arrow)
		{
			continue;
		}

		if (!FirePoint)
		{
			FirePoint = arrow;
		}

		if (arrow->GetName().Contains(TEXT("Fire")))
		{
			FirePoint = arrow;
			break;
		}
	}
}

void ACaveboundTurret::GetShotStart(FVector& shotLocation, FVector& shotForward) const
{
	// The blueprint arrow is the barrel. Without one, use the old offset and the actor front.
	if (FirePoint)
	{
		shotLocation = FirePoint->GetComponentLocation();
		shotForward = FirePoint->GetForwardVector();
		return;
	}

	shotLocation = GetActorTransform().TransformPosition(FirePointOffset);
	shotForward = GetActorForwardVector();
}

float ACaveboundTurret::GetYawErrorToEnemy(const ACaveboundBaseEnemy* enemy) const
{
	if (!enemy)
	{
		return 0.f;
	}

	FVector shotLocation;
	FVector shotForward;
	GetShotStart(shotLocation, shotForward);

	// Yaw only. Up and down do not matter for this check.
	FVector toEnemy = enemy->GetActorLocation() - shotLocation;
	toEnemy.Z = 0.f;
	shotForward.Z = 0.f;
	if (toEnemy.IsNearlyZero() || shotForward.IsNearlyZero())
	{
		return 0.f;
	}

	return FMath::FindDeltaAngleDegrees(shotForward.Rotation().Yaw, toEnemy.Rotation().Yaw);
}

void ACaveboundTurret::TurnTowardEnemy(const ACaveboundBaseEnemy* enemy, float DeltaTime)
{
	if (!enemy)
	{
		return;
	}

	// Step the yaw a little each frame so the turn is visible, then stop on the target.
	const float yawError = GetYawErrorToEnemy(enemy);
	const float step = FMath::Clamp(yawError, -TurnSpeed * DeltaTime, TurnSpeed * DeltaTime);

	FRotator rotation = GetActorRotation();
	rotation.Yaw += step;
	SetActorRotation(rotation);
}

bool ACaveboundTurret::TryFireAtEnemy(ACaveboundBaseEnemy* closestEnemy)
{
	UWorld* world = GetWorld();
	if (!closestEnemy || !world || !ProjectileClass)
	{
		return false;
	}

	// Hold the shot until the enemy is within 45 degrees of the fire point.
	if (FMath::Abs(GetYawErrorToEnemy(closestEnemy)) > AimHalfAngle)
	{
		return false;
	}

	// Spawn the shot at the fire point and aim it at the enemy's middle.
	FVector arrowSpawnPoint;
	FVector unusedForward;
	GetShotStart(arrowSpawnPoint, unusedForward);
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

		return true;
	}

	return false;
}
