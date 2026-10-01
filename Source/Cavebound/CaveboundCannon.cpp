#include "CaveboundCannon.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundCannon::ACaveboundCannon()
{
	// Bigger sphere so the cannon reads as the heavy defender.
	VisualMesh->SetRelativeScale3D(FVector(1.35f, 1.35f, 1.35f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> sphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (sphereMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(sphereMesh.Object);
	}

	// Costs the most, shoots slowly, and hits hard from far away.
	Cost = 280;
	MaxHealth = 220.f;
	Health = MaxHealth;
	AttackRange = 2000.f;
	FireInterval = 2.0f;
	ProjectileDamage = 35.f;
	ProjectileSpeed = 800.f;
}
