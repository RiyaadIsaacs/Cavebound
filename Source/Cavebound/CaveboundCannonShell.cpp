#include "CaveboundCannonShell.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundCannonShell::ACaveboundCannonShell()
{
	// A round shell. It does not need the arrow's forward tilt.
	VisualMesh->SetRelativeScale3D(FVector(0.6f));
	VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> shellMesh(
		TEXT("/Game/Assets/Models/Towers/SM_CannonShell.SM_CannonShell"));
	if (shellMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(shellMesh.Object);
	}
}
