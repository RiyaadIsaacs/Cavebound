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

void ACaveboundTurret::RememberBaseStats()
{
	if (bBaseStatsSaved)
	{
		return;
	}

	BaseMaxHealth = MaxHealth;
	BaseFireInterval = FireInterval;
	BaseProjectileDamage = ProjectileDamage;
	WoodSpent = Cost;
	bBaseStatsSaved = true;
}

void ACaveboundTurret::UseTowerMeshes(UStaticMesh* Level1, UStaticMesh* Level2, UStaticMesh* Level3, UStaticMesh* Fallback)
{
	UpgradeMeshes.Reset();

	UStaticMesh* first = Level1 ? Level1 : Fallback;
	UStaticMesh* second = Level2 ? Level2 : first;
	UStaticMesh* third = Level3 ? Level3 : second;
	if (first)
	{
		UpgradeMeshes.Add(first);
	}
	if (second)
	{
		UpgradeMeshes.Add(second);
	}
	if (third)
	{
		UpgradeMeshes.Add(third);
	}

	ApplyUpgradeMesh();
}

void ACaveboundTurret::ApplyUpgradeMesh()
{
	if (!VisualMesh || UpgradeMeshes.Num() == 0)
	{
		return;
	}

	const int32 index = FMath::Clamp(UpgradeLevel, 0, UpgradeMeshes.Num() - 1);
	if (UpgradeMeshes[index])
	{
		VisualMesh->SetStaticMesh(UpgradeMeshes[index]);
	}
}

void ACaveboundTurret::ApplyLevelBonus()
{
	const float growth = 1.f + UpgradeLevel * 0.4f;
	const float oldMax = MaxHealth;
	MaxHealth = BaseMaxHealth * growth;
	Health = FMath::Clamp(Health + FMath::Max(0.f, MaxHealth - oldMax), 1.f, MaxHealth);
	FireInterval = BaseFireInterval * FMath::Max(0.45f, 1.f - UpgradeLevel * 0.22f);
	ProjectileDamage = BaseProjectileDamage * growth;
	ApplyUpgradeMesh();
}

int32 ACaveboundTurret::GetNextUpgradeRockCost() const
{
	if (UpgradeLevel >= 2)
	{
		return 0;
	}

	if (UpgradeLevel == 0)
	{
		return FMath::Max(8, FMath::CeilToInt(Cost * 0.6f));
	}

	return FMath::Max(12, FMath::CeilToInt(Cost * 0.9f));
}

int32 ACaveboundTurret::GetNextUpgradeWoodCost() const
{
	if (UpgradeLevel != 1)
	{
		return 0;
	}

	return FMath::Max(8, FMath::CeilToInt(Cost * 0.5f));
}

int32 ACaveboundTurret::GetRepairCost() const
{
	if (Health <= 0.f || MaxHealth <= 0.f || Health >= MaxHealth)
	{
		return 0;
	}

	const float missing = 1.f - (Health / MaxHealth);
	return FMath::Max(1, FMath::CeilToInt(Cost * 0.5f * missing));
}

void ACaveboundTurret::GetDisarmRefund(int32& OutWood, int32& OutRock) const
{
	OutWood = 0;
	OutRock = 0;
	if (MaxHealth <= 0.f || Health <= 0.f)
	{
		return;
	}

	const float portion = 0.5f * FMath::Clamp(Health / MaxHealth, 0.f, 1.f);
	if (WoodSpent > 0)
	{
		OutWood = FMath::CeilToInt(WoodSpent * portion);
	}
	if (RockSpent > 0)
	{
		OutRock = FMath::CeilToInt(RockSpent * portion);
	}
}

bool ACaveboundTurret::TryUpgrade(ACaveboundGameMode* GameMode)
{
	if (!GameMode || IsDestroyed() || UpgradeLevel >= 2)
	{
		return false;
	}

	const int32 rockCost = GetNextUpgradeRockCost();
	const int32 woodCost = GetNextUpgradeWoodCost();
	if (GameMode->GetRock() < rockCost || GameMode->GetWood() < woodCost)
	{
		return false;
	}

	if (rockCost > 0 && !GameMode->UseRock(rockCost))
	{
		return false;
	}

	if (woodCost > 0 && !GameMode->UseWood(woodCost))
	{
		GameMode->GiveRock(rockCost);
		return false;
	}

	RockSpent += rockCost;
	WoodSpent += woodCost;
	++UpgradeLevel;
	ApplyLevelBonus();
	return true;
}

bool ACaveboundTurret::TryRepair(ACaveboundGameMode* GameMode)
{
	const int32 repairCost = GetRepairCost();
	if (!GameMode || repairCost <= 0 || IsDestroyed())
	{
		return false;
	}

	if (!GameMode->UseWood(repairCost))
	{
		return false;
	}

	Health = MaxHealth;
	return true;
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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> level1Mesh(TEXT("/Game/Assets/Models/Towers/SM_ArcherTower_LVL1.SM_ArcherTower_LVL1"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> level2Mesh(TEXT("/Game/Assets/Models/Towers/SM_ArcherTower_LVL2.SM_ArcherTower_LVL2"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> level3Mesh(TEXT("/Game/Assets/Models/Towers/SM_ArcherTower_LVL3.SM_ArcherTower_LVL3"));
	UseTowerMeshes(
		level1Mesh.Succeeded() ? level1Mesh.Object : nullptr,
		level2Mesh.Succeeded() ? level2Mesh.Object : nullptr,
		level3Mesh.Succeeded() ? level3Mesh.Object : nullptr,
		level1Mesh.Succeeded() ? level1Mesh.Object : nullptr);
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

	RememberBaseStats();

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
