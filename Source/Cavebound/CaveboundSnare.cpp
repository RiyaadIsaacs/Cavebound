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

	Cost = 70;
	MaxHealth = 100.f;
	Health = MaxHealth;
}

void ACaveboundSnare::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Lets the enemy move again and removes the rope.
	DropCatch();

	Super::EndPlay(EndPlayReason);
}

void ACaveboundSnare::DropCatch()
{
	for (FCaveboundSnareHold& hold : Holds)
	{
		if (ACaveboundBaseEnemy* heldEnemy = hold.Enemy.Get())
		{
			heldEnemy->ReleaseSnare(this);
		}
		ClearHold(hold);
	}
	Holds.Empty();
}

void ACaveboundSnare::ReleaseEnemy(ACaveboundBaseEnemy* Enemy)
{
	for (int32 index = Holds.Num() - 1; index >= 0; --index)
	{
		if (Holds[index].Enemy.Get() != Enemy)
		{
			continue;
		}

		if (Enemy)
		{
			Enemy->ReleaseSnare(this);
		}
		ClearHold(Holds[index]);
		Holds.RemoveAt(index);
	}
}

void ACaveboundSnare::ApplyLevelBonus()
{
	Super::ApplyLevelBonus();

	const float levelScale = 1.f + UpgradeLevel * 0.2f;
	const float postRadius = 28.f * levelScale;
	PostHeight = 700.f * levelScale;
	if (VisualMesh)
	{
		VisualMesh->SetRelativeScale3D(FVector(postRadius / 50.f, postRadius / 50.f, PostHeight / 100.f));
		VisualMesh->SetRelativeLocation(FVector(0.f, 0.f, PostHeight * 0.5f));
	}
}

void ACaveboundSnare::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsDestroyed())
	{
		return;
	}

	for (int32 index = Holds.Num() - 1; index >= 0; --index)
	{
		ACaveboundBaseEnemy* heldEnemy = Holds[index].Enemy.Get();
		if (!heldEnemy || heldEnemy->IsDead() || !heldEnemy->IsSnaredBy(this))
		{
			if (heldEnemy && !heldEnemy->IsDead())
			{
				heldEnemy->ReleaseSnare(this);
			}
			ClearHold(Holds[index]);
			Holds.RemoveAt(index);
		}
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

	const ACaveboundGameMode* gameMode = world->GetAuthGameMode<ACaveboundGameMode>();
	return !gameMode || gameMode->GetRoundState() == ECaveboundRoundState::Combat;
}

void ACaveboundSnare::UpdateSnare()
{
	while (Holds.Num() < GetCatchLimit())
	{
		ACaveboundBaseEnemy* closestEnemy = FindSnareTarget();
		if (!closestEnemy || !closestEnemy->TrySnare(this, TrapDuration))
		{
			break;
		}

		FCaveboundSnareHold hold;
		hold.Enemy = closestEnemy;
		ShowRope(hold);
		Holds.Add(hold);
	}
}

bool ACaveboundSnare::IsHolding(const ACaveboundBaseEnemy* Enemy) const
{
	for (const FCaveboundSnareHold& hold : Holds)
	{
		if (hold.Enemy.Get() == Enemy)
		{
			return true;
		}
	}

	return false;
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
		if (!enemy || !enemy->CanBeSnared() || IsHolding(enemy))
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

void ACaveboundSnare::ClearHold(FCaveboundSnareHold& Hold)
{
	for (USplineMeshComponent* ropePiece : Hold.Pieces)
	{
		if (ropePiece)
		{
			ropePiece->DestroyComponent();
		}
	}
	Hold.Pieces.Empty();

	if (Hold.Spline)
	{
		Hold.Spline->DestroyComponent();
		Hold.Spline = nullptr;
	}
}

void ACaveboundSnare::ClearAllRopes()
{
	for (FCaveboundSnareHold& hold : Holds)
	{
		ClearHold(hold);
	}
}

void ACaveboundSnare::ShowRope(FCaveboundSnareHold& Hold)
{
	ClearHold(Hold);
	ACaveboundBaseEnemy* CaughtEnemy = Hold.Enemy.Get();
	if (!CaughtEnemy || !RopeMesh)
	{
		return;
	}

	Hold.Spline = NewObject<USplineComponent>(this);
	Hold.Spline->SetMobility(EComponentMobility::Movable);
	Hold.Spline->RegisterComponent();

	const FVector postTop = GetActorLocation() + FVector(0.f, 0.f, PostHeight);
	const FVector enemyFeet = CaughtEnemy->GetActorLocation();
	const FVector aboveEnemy = enemyFeet + FVector(0.f, 0.f, 180.f);

	TArray<FVector> ropePoints;
	ropePoints.Add(postTop);
	ropePoints.Add(FMath::Lerp(postTop, aboveEnemy, 0.5f));
	ropePoints.Add(aboveEnemy);

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
		const FVector aroundBody = enemyFeet + FVector(FMath::Cos(angle) * wrapRadius, FMath::Sin(angle) * wrapRadius, heightOnBody);
		ropePoints.Add(aroundBody);
	}

	Hold.Spline->SetSplinePoints(ropePoints, ESplineCoordinateSpace::World, false);
	for (int pointIndex = 0; pointIndex < ropePoints.Num(); ++pointIndex)
	{
		Hold.Spline->SetSplinePointType(pointIndex, ESplinePointType::Curve, false);
	}
	Hold.Spline->UpdateSpline();

	const float ropeThickness = 0.08f;
	const int pointCount = Hold.Spline->GetNumberOfSplinePoints();
	for (int pointIndex = 0; pointIndex < pointCount - 1; ++pointIndex)
	{
		FVector startPoint = FVector::ZeroVector;
		FVector startDirection = FVector::ZeroVector;
		FVector endPoint = FVector::ZeroVector;
		FVector endDirection = FVector::ZeroVector;
		Hold.Spline->GetLocationAndTangentAtSplinePoint(pointIndex, startPoint, startDirection, ESplineCoordinateSpace::World);
		Hold.Spline->GetLocationAndTangentAtSplinePoint(pointIndex + 1, endPoint, endDirection, ESplineCoordinateSpace::World);

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
		Hold.Pieces.Add(ropePiece);
	}
}
