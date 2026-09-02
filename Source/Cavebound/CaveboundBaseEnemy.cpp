#include "CaveboundBaseEnemy.h"
#include "CaveboundGameMode.h"
#include "CaveboundTree.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

ACaveboundBaseEnemy::ACaveboundBaseEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create mesh
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	SetRootComponent(VisualMesh);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	VisualMesh->SetCollisionResponseToAllChannels(ECR_Block);

	Health = MaxHealth;
}

// Initialize the enemy to walk along a spline and attack the tree
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
	if (IsDead())
	{
		OnDeath();
	}
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

bool ACaveboundBaseEnemy::IsInAttackRange() const
{
	// Dist2D ignores height since tree mesh is taller than the enemy mesh
	return Tree.IsValid() && FVector::Dist2D(GetActorLocation(), Tree->GetActorLocation()) <= AttackRange;
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

	// Close enough to the tree: stop walking and chip health. Otherwise follow the spline
	if (IsInAttackRange())
	{
		AttackCurrentTarget(DeltaTime);
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
	ACaveboundTree* Target = Tree.Get();
	if (!Target || Target->IsDestroyed())
	{
		return;
	}

	// Accumulate time until AttackInterval, then deal one hit
	AttackTime += DeltaTime;
	if (AttackTime >= AttackInterval)
	{
		AttackTime = 0.f;
		Target->ApplyDamage(AttackDamage);
	}
}
