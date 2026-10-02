#pragma once

#include "CoreMinimal.h"
#include "CaveboundTurret.h"
#include "CaveboundCannon.generated.h"

/**
 * Expensive long-range defender. Fires slowly for high damage.
 */
UCLASS(Blueprintable)
class CAVEBOUND_API ACaveboundCannon : public ACaveboundTurret
{
	GENERATED_BODY()

public:
	ACaveboundCannon();

	virtual FText GetHoverDisplayName_Implementation() const override { return FText::FromString(TEXT("Cannon")); }
};
