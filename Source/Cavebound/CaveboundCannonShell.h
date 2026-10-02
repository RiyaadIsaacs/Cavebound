#pragma once

#include "CoreMinimal.h"
#include "CaveboundArrow.h"
#include "CaveboundCannonShell.generated.h"

/**
 * Cannon projectile. Same homing shot as an arrow, with the cannon shell mesh.
 */
UCLASS()
class CAVEBOUND_API ACaveboundCannonShell : public ACaveboundArrow
{
	GENERATED_BODY()

public:
	ACaveboundCannonShell();
};
