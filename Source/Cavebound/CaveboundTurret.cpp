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

int32 ACaveboundTurret::GetCostForClass(TSubclassOf<AActor> DefenderClass)
{
	UClass* Class = DefenderClass.Get();
	while (Class && Class->IsChildOf(StaticClass()))
	{
		if (!Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
		{
			if (const ACaveboundTurret* Defaults = Cast<ACaveboundTurret>(Class->GetDefaultObject()))
			{
				return Defaults->GetCost();
			}
			return 0;
		}
		Class = Class->GetSuperClass();
	}

	return 0;
}

ACaveboundTurret::ACaveboundTurret()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetRelativeScale3D(FVector(DefenderMeshScale));
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	VisualMesh->SetCollisionObjectType(ECC_WorldDynamic);
	VisualMesh->SetCollisionResponseToAllChannels(ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> towerMesh(TEXT("/Game/Assets/Models/Towers/SM_ArcherTower_LVL1.SM_ArcherTower_LVL1"));
	if (towerMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(towerMesh.Object);
	}
	VisualMesh->SetVisibility(true);
	VisualMesh->SetHiddenInGame(false);

	FirePointOffset = FVector(40.f, 0.f, 180.f);

	ProjectileClass = ACaveboundArrow::StaticClass();

	Health = MaxHealth;
}

void ACaveboundTurret::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;

	if (VisualMesh)
	{
		VisualMesh->SetRelativeScale3D(FVector(DefenderMeshScale));
		VisualMesh->SetVisibility(true);
		VisualMesh->SetHiddenInGame(false);
		if (!VisualMesh->GetStaticMesh())
		{
			if (UStaticMesh* towerMesh = LoadObject<UStaticMesh>(
					nullptr,
					TEXT("/Game/Assets/Models/Towers/SM_ArcherTower_LVL1.SM_ArcherTower_LVL1")))
			{
				VisualMesh->SetStaticMesh(towerMesh);
			}
		}
	}

	RememberFirePoint();
}

void ACaveboundTurret::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bAutoFire || IsDestroyed())
	{
		return;
	}

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
	//Enemy flash
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

	FVector arrowSpawnPoint;
	FVector unusedForward;
	GetShotStart(arrowSpawnPoint, unusedForward);
	const FVector aimAtEnemy = closestEnemy->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	const FRotator aimDirection = (aimAtEnemy - arrowSpawnPoint).Rotation();

	FActorSpawnParameters spawnSettings;
	spawnSettings.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	spawnSettings.Owner = this;
	spawnSettings.Instigator = GetInstigator();

	// Spawn the projectile
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
