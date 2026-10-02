#pragma once

#include "CoreMinimal.h"
#include "CaveboundTurret.h"
#include "CaveboundSentry.generated.h"

// Short range turret
UCLASS(Blueprintable)
class CAVEBOUND_API ACaveboundSentry : public ACaveboundTurret
{
	GENERATED_BODY()

public:
	ACaveboundSentry();
};
