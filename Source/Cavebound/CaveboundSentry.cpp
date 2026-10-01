#include "CaveboundSentry.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundSentry::ACaveboundSentry()
{
	// A thin tall cylinder. Cheap and close-range.
	VisualMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.35f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> cylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (cylinderMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(cylinderMesh.Object);
	}

	// Low cost, low health, fast weak shots at a short range.
	Cost = 80;
	MaxHealth = 80.f;
	Health = MaxHealth;
	AttackRange = 700.f;
	FireInterval = 0.35f;
	ProjectileDamage = 5.f;
	ProjectileSpeed = 1600.f;
}
