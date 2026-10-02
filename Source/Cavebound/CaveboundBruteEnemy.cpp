#include "CaveboundBruteEnemy.h"
#include "CaveboundTree.h"
#include "Engine/SkeletalMesh.h"
#include "CaveboundTurret.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundBruteEnemy::ACaveboundBruteEnemy()
{
	// Same walk, hit, punch, and death as the minion. Only the body mesh changes.
	VisualMesh->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> warriorMesh(
		TEXT("/Game/Characters/Skeleton/Skeleton_Warrior.Skeleton_Warrior"));
	if (warriorMesh.Succeeded())
	{
		BodyMesh = warriorMesh.Object;
		Body->SetSkeletalMeshAsset(BodyMesh);
	}

	MinDifficultyToSpawn = 40;
	MaxHealth = 160.f;
	Health = MaxHealth;
	MoveSpeed = 140.f;
	AttackDamage = 6.f;
	AttackRange = 320.f;
	AttackInterval = 1.35f;
	TurretDetectRange = 1100.f;
	PathHeightOffset = 0.f;
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
