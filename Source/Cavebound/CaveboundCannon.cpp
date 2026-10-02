#include "CaveboundCannon.h"
#include "CaveboundCannonShell.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundCannon::ACaveboundCannon()
{
	// Cannon tower model. Same scale as the archer tower so it fits a build slot.
	VisualMesh->SetRelativeScale3D(FVector(DefenderMeshScale));
	VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
	VisualMesh->SetRelativeLocation(FVector::ZeroVector);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> towerMesh(TEXT("/Game/Assets/Models/Towers/SM_CannonTower_LVL4.SM_CannonTower_LVL4"));
	if (towerMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(towerMesh.Object);
	}

	ProjectileClass = ACaveboundCannonShell::StaticClass();
	FirePointOffset = FVector(70.f, 0.f, 220.f);

	// Shoots slowly but long range
	Cost = 75;
	MaxHealth = 220.f;
	Health = MaxHealth;
	AttackRange = 2000.f;
	FireInterval = 2.0f;
	ProjectileDamage = 50.f;
	ProjectileSpeed = 800.f;
}
