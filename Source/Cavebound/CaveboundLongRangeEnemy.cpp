#include "CaveboundLongRangeEnemy.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundLongRangeEnemy::ACaveboundLongRangeEnemy()
{
	VisualMesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 1.6f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CylinderMesh.Object);
	}

	MinDifficultyToSpawn = 55;
	MaxHealth = 55.f;
	Health = MaxHealth;
	MoveSpeed = 220.f;
	AttackDamage = 12.f;
	// Stops well before the tree / turrets compared to melee types
	AttackRange = 900.f;
	AttackInterval = 1.1f;
	TurretDetectRange = 1000.f;
	PathHeightOffset = 70.f;
	HitFlashColor = FLinearColor(0.2f, 0.45f, 1.f);
}
