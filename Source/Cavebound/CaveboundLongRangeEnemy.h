#pragma once

#include "CoreMinimal.h"
#include "CaveboundBaseEnemy.h"
#include "CaveboundLongRangeEnemy.generated.h"

/**
 * Artillery enemy: stops farther out and attacks with a long AttackRange.
 */
UCLASS()
class CAVEBOUND_API ACaveboundLongRangeEnemy : public ACaveboundBaseEnemy
{
	GENERATED_BODY()

public:
	ACaveboundLongRangeEnemy();
};
