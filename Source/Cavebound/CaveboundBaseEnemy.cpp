#include "CaveboundBaseEnemy.h"
#include "CaveboundGameMode.h"
#include "CaveboundTree.h"
#include "CaveboundTurret.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ACaveboundBaseEnemy::ACaveboundBaseEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create mesh
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	SetRootComponent(VisualMesh);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	VisualMesh->SetCollisionObjectType(ECC_WorldDynamic);
	VisualMesh->SetCollisionResponseToAllChannels(ECR_Block);
	VisualMesh->SetGenerateOverlapEvents(true);

	Health = MaxHealth;
}

// Initialize the enemy to walk along a spline 
void ACaveboundBaseEnemy::InitAlongPath(USplineComponent* Spline, ACaveboundTree* InTree)
{
	PathSpline = Spline;
	Tree = InTree;
	DistanceAlongSpline = 0.f;
	AttackTime = 0.f;

	if (Spline)
	{
		const FVector Start = Spline->GetLocationAtDistanceAlongSpline(0.f, ESplineCoordinateSpace::World);
		SetActorLocation(Start + FVector(0.f, 0.f, PathHeightOffset));
	}
}

void ACaveboundBaseEnemy::ApplyDamage(float Amount)
{
	if (IsDead() || Amount <= 0.f)
	{
		return;
	}

	Health = FMath::Max(0.f, Health - Amount);
	PlayDamageFlash();

	if (IsDead())
	{
		OnDeath();
	}
}

void ACaveboundBaseEnemy::PlayDamageFlash()
{
	TArray<UStaticMeshComponent*> Meshes;
	if (VisualMesh)
	{
		Meshes.Add(VisualMesh);
	}

	FCaveboundDamageFlash::FlashMeshes(this, Meshes, HitFlashTimer, HitFlashColor, HitFlashDuration);
}

void ACaveboundBaseEnemy::OnDeath()
{
	// Notify the game mode of enemy defetead 
	if (UWorld* World = GetWorld())
	{
		if (ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>())
		{
			GameMode->RegisterEnemyDefeated();
		}
	}

	Destroy();
}

ACaveboundTurret* ACaveboundBaseEnemy::FindNearestTurretInRange(float Range) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> FoundTurrets;
	UGameplayStatics::GetAllActorsOfClass(World, ACaveboundTurret::StaticClass(), FoundTurrets);

	ACaveboundTurret* Nearest = nullptr;
	float BestDistance = Range;
	const FVector MyLocation = GetActorLocation();

	for (AActor* Actor : FoundTurrets)
	{
		ACaveboundTurret* Turret = Cast<ACaveboundTurret>(Actor);
		if (!Turret || Turret->IsDestroyed())
		{
			continue;
		}

		const float Distance = FVector::Dist2D(MyLocation, Turret->GetActorLocation());
		if (Distance <= BestDistance)
		{
			BestDistance = Distance;
			Nearest = Turret;
		}
	}

	return Nearest;
}

AActor* ACaveboundBaseEnemy::ResolveAttackTarget() const
{
	// Target turrets first 
	if (ACaveboundTurret* Turret = FindNearestTurretInRange(TurretDetectRange))
	{
		return Turret;
	}

	ACaveboundTree* CurrentTree = Tree.Get();
	if (CurrentTree && !CurrentTree->IsDestroyed()
		&& IsInAttackRangeOf(CurrentTree))
	{
		return CurrentTree;
	}

	return nullptr;
}

bool ACaveboundBaseEnemy::IsInAttackRangeOf(const AActor* Target) const
{
	if (!IsValid(Target))
	{
		return false;
	}

	// Dist2D ignores height differences between meshes
	return FVector::Dist2D(GetActorLocation(), Target->GetActorLocation()) <= AttackRange;
}

void ACaveboundBaseEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsDead())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (const ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>())
		{
			if (GameMode->IsGameOver())
			{
				return;
			}
		}
	}

	if (AActor* Target = ResolveAttackTarget())
	{
		if (IsInAttackRangeOf(Target))
		{
			AttackCurrentTarget(DeltaTime);
			return;
		}

		// Close enough to notice a turret, but still need to walk up to it
		FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0.f;
		if (!ToTarget.IsNearlyZero())
		{
			AddActorWorldOffset(ToTarget.GetSafeNormal() * MoveSpeed * DeltaTime);
			SetActorRotation(ToTarget.Rotation());
		}
		return;
	}

	MoveAlongPath(DeltaTime);
}

void ACaveboundBaseEnemy::MoveAlongPath(float DeltaTime)
{
	USplineComponent* Spline = PathSpline.Get();
	if (!Spline)
	{
		return;
	}

	const float SplineLength = Spline->GetSplineLength();
	if (DistanceAlongSpline >= SplineLength)
	{
		// Path ended before the trunk
		if (Tree.IsValid())
		{
			FVector ToTree = Tree->GetActorLocation() - GetActorLocation();
			ToTree.Z = 0.f;
			if (ToTree.Size() > AttackRange)
			{
				AddActorWorldOffset(ToTree.GetSafeNormal() * MoveSpeed * DeltaTime);
			}
		}
		return;
	}

	// Move along the spline and don't overshoot the end
	DistanceAlongSpline = FMath::Min(DistanceAlongSpline + MoveSpeed * DeltaTime, SplineLength);

	const FVector PathPoint = Spline->GetLocationAtDistanceAlongSpline(
		DistanceAlongSpline,
		ESplineCoordinateSpace::World);
	SetActorLocation(PathPoint + FVector(0.f, 0.f, PathHeightOffset));

	const FVector Tangent = Spline->GetTangentAtDistanceAlongSpline(
		DistanceAlongSpline,
		ESplineCoordinateSpace::World);
	if (!Tangent.IsNearlyZero())
	{
		SetActorRotation(Tangent.Rotation());
	}
}

void ACaveboundBaseEnemy::AttackCurrentTarget(float DeltaTime)
{
	AActor* Target = ResolveAttackTarget();
	if (!IsValid(Target))
	{
		return;
	}

	FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	if (!ToTarget.IsNearlyZero())
	{
		SetActorRotation(ToTarget.Rotation());
	}

	// Accumulate time until AttackInterval, then deal one hit
	AttackTime += DeltaTime;
	if (AttackTime < AttackInterval)
	{
		return;
	}

	AttackTime = 0.f;

	if (ACaveboundTurret* Turret = Cast<ACaveboundTurret>(Target))
	{
		Turret->ApplyDamage(AttackDamage);
		return;
	}

	if (ACaveboundTree* TreeTarget = Cast<ACaveboundTree>(Target))
	{
		TreeTarget->ApplyDamage(AttackDamage);
	}
}
