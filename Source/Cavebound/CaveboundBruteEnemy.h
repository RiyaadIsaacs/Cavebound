#pragma once

#include "CoreMinimal.h"
#include "CaveboundBaseEnemy.h"
#include "CaveboundBruteEnemy.generated.h"

/**
 * Slow siege enemy: high HP/damage, prefers turrets over the tree.
 */
UCLASS()
class CAVEBOUND_API ACaveboundBruteEnemy : public ACaveboundBaseEnemy
{
	GENERATED_BODY()

public:
	ACaveboundBruteEnemy();

protected:
	virtual AActor* ResolveAttackTarget() const override;
};
