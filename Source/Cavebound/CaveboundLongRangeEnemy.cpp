#include "CaveboundLongRangeEnemy.h"
#include "CaveboundEnemyBolt.h"
#include "Animation/AnimMontage.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundLongRangeEnemy::ACaveboundLongRangeEnemy()
{
	// Same walk, hit, and death as the minion. The attack clip is a ranged cast instead of a punch.
	VisualMesh->SetRelativeScale3D(FVector(1.f, 1.f, 1.f));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> mageMesh(
		TEXT("/Game/Characters/Skeleton/Skeleton_Mage.Skeleton_Mage"));
	if (mageMesh.Succeeded())
	{
		BodyMesh = mageMesh.Object;
		Body->SetSkeletalMeshAsset(BodyMesh);
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> rangedAttack(
		TEXT("/Game/Characters/Skeleton/AM_MageRanged.AM_MageRanged"));
	if (rangedAttack.Succeeded())
	{
		MeleeMontage = rangedAttack.Object;
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
	PathHeightOffset = 0.f;
	HitFlashColor = FLinearColor(0.2f, 0.45f, 1.f);
}

bool ACaveboundLongRangeEnemy::AttackAppliesDamageNow() const
{
	return false;
}

void ACaveboundLongRangeEnemy::OnMeleeStrike()
{
	Super::OnMeleeStrike();
	FireBolt(ResolveAttackTarget());
}

void ACaveboundLongRangeEnemy::FireBolt(AActor* Target)
{
	if (!Target)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Leave the hands, then the bolt flies at the target and deals the hit there.
	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 140.f);
	ACaveboundEnemyBolt* Bolt = World->SpawnActor<ACaveboundEnemyBolt>(
		ACaveboundEnemyBolt::StaticClass(),
		Start,
		FRotator::ZeroRotator);
	if (Bolt)
	{
		Bolt->Init(Target, AttackDamage);
	}
}
