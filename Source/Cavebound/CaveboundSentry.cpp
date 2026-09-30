#include "CaveboundSentry.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundSentry::ACaveboundSentry()
{
	VisualMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.35f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CylinderMesh.Object);
	}

	Cost = 80;
	MaxHealth = 80.f;
	Health = MaxHealth;
	AttackRange = 700.f;
	FireInterval = 0.35f;
	ProjectileDamage = 5.f;
	ProjectileSpeed = 1600.f;
}
