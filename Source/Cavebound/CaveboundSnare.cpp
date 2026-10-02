#include "CaveboundSnare.h"

#include "CaveboundBaseEnemy.h"
#include "CaveboundGameMode.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundSnare::ACaveboundSnare()
{
	bAutoFire = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> cylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (cylinderMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(cylinderMesh.Object);
		RopeMesh = cylinderMesh.Object;
	}

	// Mesh dimensions
	const float postRadius = 28.f;
	VisualMesh->SetRelativeScale3D(FVector(postRadius / 50.f, postRadius / 50.f, PostHeight / 100.f));
	VisualMesh->SetRelativeLocation(FVector(0.f, 0.f, PostHeight * 0.5f));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ropeColorMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (ropeColorMaterial.Succeeded())
	{
		RopeMaterial = UMaterialInstanceDynamic::Create(ropeColorMaterial.Object, this);
		if (RopeMaterial)
		{
			RopeMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.45f, 0.28f, 0.12f, 1.f));
		}
	}

	Cost = 140;
	MaxHealth = 100.f;
	Health = MaxHealth;
}

void ACaveboundSnare::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// If the trap is removed while it is holding someone, let that enemy walk again.
	if (ACaveboundBaseEnemy* heldEnemy = TrappedEnemy.Get())
	{
		heldEnemy->ReleaseSnare(this);
	}
	TrappedEnemy = nullptr;
	ClearRope();

	Super::EndPlay(EndPlayReason);
}

void ACaveboundSnare::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsDestroyed())
	{
		return;
	}

	if (!IsCombatRound())
	{
		return;
	}

	UpdateSnare();
}

bool ACaveboundSnare::IsCombatRound() const
{
	const UWorld* world = GetWorld();
	if (!world)
	{
		return false;
	}

	// If there is no game mode, still allow the trap. Otherwise only catch during combat.
	const ACaveboundGameMode* gameMode = world->GetAuthGameMode<ACaveboundGameMode>();
	return !gameMode || gameMode->GetRoundState() == ECaveboundRoundState::Combat;
}

void ACaveboundSnare::UpdateSnare()
{
	// Keep the current catch if that enemy is still held and still alive.
	if (ACaveboundBaseEnemy* heldEnemy = TrappedEnemy.Get())
	{
		if (heldEnemy->IsSnaredBy(this) && !heldEnemy->IsDead())
		{
			if (RopePieces.Num() == 0)
			{
				ShowRope(heldEnemy);
			}
			return;
		}

		TrappedEnemy = nullptr;
		ClearRope();
	}

	// Snares enemy that hasnt been snared
	if (ACaveboundBaseEnemy* closestEnemy = FindSnareTarget())
	{
		if (closestEnemy->TrySnare(this, TrapDuration))
		{
			TrappedEnemy = closestEnemy;
			ShowRope(closestEnemy);
		}
	}
}

ACaveboundBaseEnemy* ACaveboundSnare::FindSnareTarget() const
{
	UWorld* world = GetWorld();
	if (!world)
	{
		return nullptr;
	}

	TArray<AActor*> allEnemies;
	UGameplayStatics::GetAllActorsOfClass(world, ACaveboundBaseEnemy::StaticClass(), allEnemies);

	ACaveboundBaseEnemy* closestEnemy = nullptr;
	float closestDistance = CatchRange;
	const FVector trapLocation = GetActorLocation();

	for (AActor* foundActor : allEnemies)
	{
		ACaveboundBaseEnemy* enemy = Cast<ACaveboundBaseEnemy>(foundActor);
		if (!enemy || !enemy->CanBeSnared())
		{
			continue;
		}

		const float distanceToEnemy = FVector::Dist2D(trapLocation, enemy->GetActorLocation());
		if (distanceToEnemy <= closestDistance)
		{
			closestDistance = distanceToEnemy;
			closestEnemy = enemy;
		}
	}

	return closestEnemy;
}

void ACaveboundSnare::ClearRope()
{
	for (USplineMeshComponent* ropePiece : RopePieces)
	{
		if (ropePiece)
		{
			ropePiece->DestroyComponent();
		}
	}
	RopePieces.Empty();

	if (RopeSpline)
	{
		RopeSpline->DestroyComponent();
		RopeSpline = nullptr;
	}
}

void ACaveboundSnare::ShowRope(ACaveboundBaseEnemy* CaughtEnemy)
{
	ClearRope();
	if (!CaughtEnemy || !RopeMesh)
	{
		return;
	}

	RopeSpline = NewObject<USplineComponent>(this);
	RopeSpline->SetMobility(EComponentMobility::Movable);
	RopeSpline->RegisterComponent();

	const FVector postTop = GetActorLocation() + FVector(0.f, 0.f, PostHeight);
	const FVector enemyFeet = CaughtEnemy->GetActorLocation();
	const FVector aboveEnemy = enemyFeet + FVector(0.f, 0.f, 180.f);

	TArray<FVector> ropePoints;
	ropePoints.Add(postTop);
	ropePoints.Add(FMath::Lerp(postTop, aboveEnemy, 0.5f));
	ropePoints.Add(aboveEnemy);

	// Spline wraps around enemy
	const int wraps = 3;
	const int stepsPerWrap = 8;
	const float wrapRadius = 80.f;
	const float topOfWrap = 170.f;
	const float bottomOfWrap = 15.f;
	const int wrapSteps = wraps * stepsPerWrap;
	for (int step = 0; step <= wrapSteps; ++step)
	{
		const float alongWrap = static_cast<float>(step) / static_cast<float>(wrapSteps);
		const float angle = alongWrap * wraps * 2.f * PI;
		const float heightOnBody = FMath::Lerp(topOfWrap, bottomOfWrap, alongWrap);
		const FVector aroundBody = enemyFeet + FVector(
			FMath::Cos(angle) * wrapRadius,
			FMath::Sin(angle) * wrapRadius,
			heightOnBody);
		ropePoints.Add(aroundBody);
	}

	RopeSpline->SetSplinePoints(ropePoints, ESplineCoordinateSpace::World, false);
	for (int pointIndex = 0; pointIndex < ropePoints.Num(); ++pointIndex)
	{
		RopeSpline->SetSplinePointType(pointIndex, ESplinePointType::Curve, false);
	}
	RopeSpline->UpdateSpline();

	// Builds mesh along spline
	const float ropeThickness = 0.08f;
	const int pointCount = RopeSpline->GetNumberOfSplinePoints();
	for (int pointIndex = 0; pointIndex < pointCount - 1; ++pointIndex)
	{
		FVector startPoint = FVector::ZeroVector;
		FVector startDirection = FVector::ZeroVector;
		FVector endPoint = FVector::ZeroVector;
		FVector endDirection = FVector::ZeroVector;
		RopeSpline->GetLocationAndTangentAtSplinePoint(pointIndex, startPoint, startDirection, ESplineCoordinateSpace::World);
		RopeSpline->GetLocationAndTangentAtSplinePoint(pointIndex + 1, endPoint, endDirection, ESplineCoordinateSpace::World);

		USplineMeshComponent* ropePiece = NewObject<USplineMeshComponent>(this);
		ropePiece->SetMobility(EComponentMobility::Movable);
		ropePiece->SetStaticMesh(RopeMesh);
		ropePiece->SetForwardAxis(ESplineMeshAxis::Z, false);
		ropePiece->SetStartAndEnd(startPoint, startDirection, endPoint, endDirection, true);
		ropePiece->SetStartScale(FVector2D(ropeThickness, ropeThickness), false);
		ropePiece->SetEndScale(FVector2D(ropeThickness, ropeThickness), true);
		ropePiece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ropePiece->SetCastShadow(false);
		if (RopeMaterial)
		{
			ropePiece->SetMaterial(0, RopeMaterial);
		}
		ropePiece->RegisterComponent();
		RopePieces.Add(ropePiece);
	}
}
