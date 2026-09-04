#include "CaveboundEnemy.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundEnemy::ACaveboundEnemy()
{
	// Slightly taller cone so it reads as a pawn
	VisualMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.1f));

	// Load a basic cone mesh from the engine content
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(ConeMesh.Object);
	}

	// Defaults for this enemy type
	MaxHealth = 60.f;
	Health = MaxHealth;
	MoveSpeed = 280.f;
	AttackDamage = 8.f;
	AttackRange = 280.f;
	AttackInterval = 1.0f;
}
