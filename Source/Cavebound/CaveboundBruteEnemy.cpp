#include "CaveboundBruteEnemy.h"
#include "CaveboundTree.h"
#include "CaveboundTurret.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundBruteEnemy::ACaveboundBruteEnemy()
{
	VisualMesh->SetRelativeScale3D(FVector(1.1f, 1.1f, 0.85f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMesh.Object);
	}

	MinDifficultyToSpawn = 40;
	MaxHealth = 160.f;
	Health = MaxHealth;
	MoveSpeed = 140.f;
	AttackDamage = 6.f;
	AttackRange = 320.f;
	AttackInterval = 1.35f;
	TurretDetectRange = 1100.f;
	PathHeightOffset = 55.f;
	HitFlashColor = FLinearColor(0.55f, 0.15f, 0.05f);
}

AActor* ACaveboundBruteEnemy::ResolveAttackTarget() const
{
	// Target the nearest turret in a wide radius before the tree
	if (ACaveboundTurret* Turret = FindNearestTurretInRange(TurretDetectRange))
	{
		return Turret;
	}

	// Only attack the tree once no turrets remain in detect range
	ACaveboundTree* CurrentTree = Tree.Get();
	if (CurrentTree && !CurrentTree->IsDestroyed() && IsInAttackRangeOf(CurrentTree))
	{
		return CurrentTree;
	}

	return nullptr;
}
