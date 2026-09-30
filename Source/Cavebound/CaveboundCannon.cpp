#include "CaveboundCannon.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundCannon::ACaveboundCannon()
{
	VisualMesh->SetRelativeScale3D(FVector(1.35f, 1.35f, 1.35f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(SphereMesh.Object);
	}

	Cost = 280;
	MaxHealth = 220.f;
	Health = MaxHealth;
	AttackRange = 2000.f;
	FireInterval = 2.0f;
	ProjectileDamage = 35.f;
	ProjectileSpeed = 800.f;
}
