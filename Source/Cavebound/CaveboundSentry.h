#pragma once

#include "CoreMinimal.h"
#include "CaveboundTurret.h"
#include "CaveboundSentry.generated.h"

/**
 * Cheap short-range defender. Fires quickly for low damage.
 */
UCLASS(Blueprintable)
class CAVEBOUND_API ACaveboundSentry : public ACaveboundTurret
{
	GENERATED_BODY()

public:
	ACaveboundSentry();

	virtual FText GetHoverDisplayName_Implementation() const override { return FText::FromString(TEXT("Sentry")); }
};
