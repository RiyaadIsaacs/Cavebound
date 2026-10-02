#include "CaveboundBallistaShell.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundBallistaShell::ACaveboundBallistaShell()
{
	VisualMesh->SetRelativeScale3D(FVector(0.4f));
	VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> shellMesh(
		TEXT("/Game/Assets/Models/Towers/SM_BallistaShell.SM_BallistaShell"));
	if (shellMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(shellMesh.Object);
	}
}
