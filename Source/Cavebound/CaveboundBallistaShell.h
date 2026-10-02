#pragma once

#include "CoreMinimal.h"
#include "CaveboundArrow.h"
#include "CaveboundBallistaShell.generated.h"

/**
 * Sentry projectile. Same homing shot as an arrow, with the ballista bolt mesh.
 */
UCLASS()
class CAVEBOUND_API ACaveboundBallistaShell : public ACaveboundArrow
{
	GENERATED_BODY()

public:
	ACaveboundBallistaShell();
};
