#include "CaveboundSentry.h"
#include "CaveboundBallistaShell.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundSentry::ACaveboundSentry()
{
	VisualMesh->SetRelativeScale3D(FVector(0.2f));
	VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
	VisualMesh->SetRelativeLocation(FVector::ZeroVector);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> towerMesh(TEXT("/Game/Assets/Models/Towers/SM_BallistaTower_LVL4.SM_BallistaTower_LVL4"));
	if (towerMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(towerMesh.Object);
	}

	ProjectileClass = ACaveboundBallistaShell::StaticClass();
	FirePointOffset = FVector(70.f, 0.f, 240.f);

	Cost = 50;
	MaxHealth = 50.f;
	Health = MaxHealth;
	AttackRange = 700.f;
	FireInterval = 0.35f;
	ProjectileDamage = 5.f;
	ProjectileSpeed = 1600.f;
}
