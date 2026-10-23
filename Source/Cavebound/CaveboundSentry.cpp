#include "CaveboundSentry.h"
#include "CaveboundBallistaShell.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundSentry::ACaveboundSentry()
{
	VisualMesh->SetRelativeScale3D(FVector(DefenderMeshScale));
	VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
	VisualMesh->SetRelativeLocation(FVector::ZeroVector);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> level1Mesh(TEXT("/Game/Assets/Models/Towers/SM_BallistaTower_LVL1.SM_BallistaTower_LVL1"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> level2Mesh(TEXT("/Game/Assets/Models/Towers/SM_BallistaTower_LVL2.SM_BallistaTower_LVL2"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> level3Mesh(TEXT("/Game/Assets/Models/Towers/SM_BallistaTower_LVL3.SM_BallistaTower_LVL3"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> fallbackMesh(TEXT("/Game/Assets/Models/Towers/SM_BallistaTower_LVL4.SM_BallistaTower_LVL4"));
	UseTowerMeshes(
		level1Mesh.Succeeded() ? level1Mesh.Object : nullptr,
		level2Mesh.Succeeded() ? level2Mesh.Object : nullptr,
		level3Mesh.Succeeded() ? level3Mesh.Object : nullptr,
		fallbackMesh.Succeeded() ? fallbackMesh.Object : nullptr);

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
