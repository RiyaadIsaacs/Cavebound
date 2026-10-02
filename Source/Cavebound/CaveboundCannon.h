#pragma once

#include "CoreMinimal.h"
#include "CaveboundTurret.h"
#include "CaveboundCannon.generated.h"

// Long range defender
UCLASS(Blueprintable)
class CAVEBOUND_API ACaveboundCannon : public ACaveboundTurret
{
	GENERATED_BODY()

public:
	ACaveboundCannon();

	virtual FText GetHoverDisplayName_Implementation() const override { return FText::FromString(TEXT("Cannon")); }
};
