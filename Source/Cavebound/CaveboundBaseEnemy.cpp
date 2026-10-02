#include "CaveboundBaseEnemy.h"
#include "CaveboundGameMode.h"
#include "CaveboundTree.h"
#include "CaveboundTurret.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

ACaveboundBaseEnemy::ACaveboundBaseEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	SetRootComponent(VisualMesh);
	// Enemies Block WorldDynamic by default, which turns arrow Overlap into Block and
	// kills BeginOverlap. Overlap WorldDynamic so projectiles can register hits.
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	VisualMesh->SetCollisionObjectType(ECC_WorldDynamic);
	VisualMesh->SetCollisionResponseToAllChannels(ECR_Block);
	VisualMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
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

void ACaveboundBaseEnemy::ApplySlow(float SpeedMultiplier)
{
	UWorld* World = GetWorld();
	if (!World || IsDead())
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	if (Now - SlowRefreshTime > 0.2f)
	{
		ActiveSlowMultiplier = 1.f;
	}

	ActiveSlowMultiplier = FMath::Clamp(FMath::Min(ActiveSlowMultiplier, SpeedMultiplier), 0.f, 1.f);
	SlowRefreshTime = Now;
}

float ACaveboundBaseEnemy::GetMoveSpeedScale() const
{
	const UWorld* World = GetWorld();
	if (!World || World->GetTimeSeconds() - SlowRefreshTime > 0.2f)
	{
		return 1.f;
	}

	return ActiveSlowMultiplier;
}

bool ACaveboundBaseEnemy::CanBeSnared() const
{
	return !IsDead() && !bIsSnared && !bHasBeenSnared;
}

bool ACaveboundBaseEnemy::TrySnare(AActor* Source, float Duration)
{
	if (!CanBeSnared() || !IsValid(Source) || Duration <= 0.f)
	{
		return false;
	}

	bIsSnared = true;
	bHasBeenSnared = true;
	SnareSource = Source;
	SnareTimeRemaining = Duration;
	return true;
}

void ACaveboundBaseEnemy::ReleaseSnare(AActor* Source)
{
	if (SnareSource.Get() != Source)
	{
		return;
	}

	bIsSnared = false;
	SnareSource = nullptr;
	SnareTimeRemaining = 0.f;
}

bool ACaveboundBaseEnemy::IsSnaredBy(const AActor* Source) const
{
	return bIsSnared && SnareSource.Get() == Source;
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
	if (CurrentTree && !CurrentTree->IsDestroyed() && IsInAttackRangeOf(CurrentTree))
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

float ACaveboundBaseEnemy::ComputeBoidLaneOffset(const FVector& PathPoint, const FVector& PathRight) const
{
	if (!bUseBoids || PathRight.IsNearlyZero())
	{
		return 0.f;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}

	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsOfClass(World, ACaveboundBaseEnemy::StaticClass(), FoundEnemies);

	FVector Separation = FVector::ZeroVector;
	FVector Alignment = FVector::ZeroVector;
	FVector CohesionSum = FVector::ZeroVector;
	int32 NeighbourCount = 0;

	const FVector MyLocation = GetActorLocation();

	for (AActor* Actor : FoundEnemies)
	{
		ACaveboundBaseEnemy* Other = Cast<ACaveboundBaseEnemy>(Actor);
		if (!Other || Other == this || Other->IsDead())
		{
			continue;
		}

		const FVector ToOther = Other->GetActorLocation() - MyLocation;
		const float Dist = ToOther.Size2D();
		if (Dist > SeparationRadius || Dist < KINDA_SMALL_NUMBER)
		{
			continue;
		}

		++NeighbourCount;

		// Stronger push when closer
		const float Push = (SeparationRadius - Dist) / SeparationRadius;
		Separation -= FVector(ToOther.X, ToOther.Y, 0.f).GetSafeNormal() * Push;

		Alignment += Other->GetActorForwardVector();
		CohesionSum += Other->GetActorLocation();
	}

	if (NeighbourCount == 0)
	{
		return 0.f;
	}

	Alignment /= static_cast<float>(NeighbourCount);
	const FVector Cohesion = ((CohesionSum / static_cast<float>(NeighbourCount)) - MyLocation).GetSafeNormal2D();

	FVector Combined =
		Separation * SeparationStrength
		+ Alignment.GetSafeNormal2D() * AlignmentStrength
		+ Cohesion * CohesionStrength;

	Combined.Z = 0.f;
	const float LaneOffset = FVector::DotProduct(Combined, PathRight);
	return FMath::Clamp(LaneOffset * SeparationRadius, -MaxLaneOffset, MaxLaneOffset);
}

float ACaveboundBaseEnemy::ComputeForwardStagger() const
{
	if (!bUseBoids || ForwardStaggerDistance <= 0.f)
	{
		return 1.f;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return 1.f;
	}

	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsOfClass(World, ACaveboundBaseEnemy::StaticClass(), FoundEnemies);

	float SpeedMul = 1.f;
	for (AActor* Actor : FoundEnemies)
	{
		ACaveboundBaseEnemy* Other = Cast<ACaveboundBaseEnemy>(Actor);
		if (!Other || Other == this || Other->IsDead())
		{
			continue;
		}

		// Only stagger against enemies on the same spline
		if (Other->PathSpline.Get() != PathSpline.Get())
		{
			continue;
		}

		const float DistDelta = Other->GetDistanceAlongSpline() - DistanceAlongSpline;
		if (FMath::Abs(DistDelta) < ForwardStaggerDistance)
		{
			// If nearly tied or slightly ahead of us, ease off so packs desync
			if (DistDelta >= -KINDA_SMALL_NUMBER)
			{
				SpeedMul = FMath::Min(SpeedMul, 1.f - ForwardStaggerStrength);
			}
		}
	}

	return FMath::Clamp(SpeedMul, 0.4f, 1.f);
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

	if (bIsSnared)
	{
		if (!SnareSource.IsValid())
		{
			bIsSnared = false;
			SnareTimeRemaining = 0.f;
		}
		else
		{
			SnareTimeRemaining -= DeltaTime;
			if (SnareTimeRemaining <= 0.f)
			{
				bIsSnared = false;
				SnareSource = nullptr;
				SnareTimeRemaining = 0.f;
			}
			else
			{
				return;
			}
		}
	}

	const float SpeedScale = GetMoveSpeedScale();

	if (AActor* Target = ResolveAttackTarget())
	{
		// Walk to an open spot in range, then attack from there.
		const FVector Spot = ChooseAttackSpot(Target);
		FVector ToSpot = Spot - GetActorLocation();
		ToSpot.Z = 0.f;

		if (ToSpot.Size() > 40.f)
		{
			AddActorWorldOffset(ToSpot.GetSafeNormal() * MoveSpeed * SpeedScale * DeltaTime);
			SetActorRotation(ToSpot.Rotation());
			return;
		}

		if (IsInAttackRangeOf(Target))
		{
			AttackCurrentTarget(DeltaTime);
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
	const float SpeedScale = GetMoveSpeedScale() * ComputeForwardStagger();

	if (DistanceAlongSpline >= SplineLength)
	{
		// Path ended before the trunk
		if (Tree.IsValid())
		{
			FVector ToTree = Tree->GetActorLocation() - GetActorLocation();
			ToTree.Z = 0.f;
			if (ToTree.Size() > AttackRange)
			{
				AddActorWorldOffset(ToTree.GetSafeNormal() * MoveSpeed * SpeedScale * DeltaTime);
			}
		}
		return;
	}

	// Off the path after a fight. Walk to the closest point on it, not back to the spawn.
	const float ClosestKey = Spline->FindInputKeyClosestToWorldLocation(GetActorLocation());
	const float ClosestDistance = Spline->GetDistanceAlongSplineAtSplineInputKey(ClosestKey);
	const FVector ClosestPoint = Spline->GetLocationAtDistanceAlongSpline(
		ClosestDistance,
		ESplineCoordinateSpace::World);
	const FVector ClosestSpot = ClosestPoint + FVector(0.f, 0.f, PathHeightOffset);

	FVector ToPath = ClosestSpot - GetActorLocation();
	ToPath.Z = 0.f;
	if (ToPath.Size() > MaxLaneOffset + 80.f)
	{
		AddActorWorldOffset(ToPath.GetSafeNormal() * MoveSpeed * GetMoveSpeedScale() * DeltaTime);
		SetActorRotation(ToPath.Rotation());
		return;
	}

	// Back beside the path. Keep going from here.
	DistanceAlongSpline = FMath::Clamp(ClosestDistance, 0.f, SplineLength);

	// Move along the spline and don't overshoot the end
	DistanceAlongSpline = FMath::Min(DistanceAlongSpline + MoveSpeed * SpeedScale * DeltaTime, SplineLength);

	const FVector PathPoint = Spline->GetLocationAtDistanceAlongSpline(
		DistanceAlongSpline,
		ESplineCoordinateSpace::World);

	FVector Tangent = Spline->GetTangentAtDistanceAlongSpline(
		DistanceAlongSpline,
		ESplineCoordinateSpace::World);
	Tangent.Z = 0.f;

	FVector PathRight = FVector::CrossProduct(FVector::UpVector, Tangent.GetSafeNormal());
	if (PathRight.IsNearlyZero())
	{
		PathRight = FVector::RightVector;
	}
	else
	{
		PathRight.Normalize();
	}

	const float LaneOffset = ComputeBoidLaneOffset(PathPoint, PathRight);
	SetActorLocation(PathPoint + PathRight * LaneOffset + FVector(0.f, 0.f, PathHeightOffset));

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

	// A hit animation says no, so do not start a new swing yet.
	if (!CanAttack())
	{
		return;
	}

	// Accumulate time until AttackInterval, then deal one hit
	AttackTime += DeltaTime;
	if (AttackTime < AttackInterval)
	{
		return;
	}

	AttackTime = 0.f;

	// Play the swing. Melee hits damage now. A ranged hit spawns its own projectile.
	OnMeleeStrike();
	if (!AttackAppliesDamageNow())
	{
		return;
	}

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

bool ACaveboundBaseEnemy::IsSpotBlockedByEnemy(const FVector& Spot) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// Another living enemy is already using this place.
	for (TActorIterator<ACaveboundBaseEnemy> It(World); It; ++It)
	{
		const ACaveboundBaseEnemy* Other = *It;
		if (!Other || Other == this || Other->IsDead())
		{
			continue;
		}

		if (FVector::Dist2D(Spot, Other->GetActorLocation()) < StandClearDistance)
		{
			return true;
		}
	}

	return false;
}

FVector ACaveboundBaseEnemy::ChooseAttackSpot(const AActor* Target) const
{
	if (!Target)
	{
		return GetActorLocation();
	}

	// Already in range and not standing on someone.
	if (IsInAttackRangeOf(Target) && !IsSpotBlockedByEnemy(GetActorLocation()))
	{
		return GetActorLocation();
	}

	const FVector TargetLocation = Target->GetActorLocation();
	const float StandDistance = FMath::Max(80.f, AttackRange * 0.7f);
	const int32 SlotCount = 8;
	const int32 FirstSlot = GetUniqueID() % SlotCount;

	for (int32 Step = 0; Step < SlotCount; ++Step)
	{
		const int32 Slot = (FirstSlot + Step) % SlotCount;
		const float Angle = (2.f * PI * Slot) / SlotCount;
		FVector Spot = TargetLocation + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * StandDistance;
		Spot.Z = GetActorLocation().Z;

		if (!IsSpotBlockedByEnemy(Spot))
		{
			return Spot;
		}
	}

	// Every seat is taken. Stay here if we can already hit, otherwise walk toward the target and wait.
	if (IsInAttackRangeOf(Target))
	{
		return GetActorLocation();
	}

	FVector TowardTarget = TargetLocation - GetActorLocation();
	TowardTarget.Z = 0.f;
	if (TowardTarget.IsNearlyZero())
	{
		return GetActorLocation();
	}

	FVector Spot = TargetLocation - TowardTarget.GetSafeNormal() * StandDistance;
	Spot.Z = GetActorLocation().Z;
	return Spot;
}
