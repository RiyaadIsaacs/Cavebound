#pragma once

#include "CoreMinimal.h"
#include "CaveboundTurret.h"
#include "CaveboundSentry.generated.h"

/**
 * Cheap short-range defender
 */
UCLASS(Blueprintable)
class CAVEBOUND_API ACaveboundSentry : public ACaveboundTurret
{
	GENERATED_BODY()

public:
	ACaveboundSentry();
};
