#pragma once

#include "CoreMinimal.h"
#include "CaveboundEnemy.h"
#include "CaveboundBruteEnemy.generated.h"

/**
 * Slow siege enemy: high HP/damage, prefers turrets over the tree.
 */
UCLASS()
class CAVEBOUND_API ACaveboundBruteEnemy : public ACaveboundEnemy
{
	GENERATED_BODY()

public:
	ACaveboundBruteEnemy();

protected:
	virtual AActor* ResolveAttackTarget() const override;
};
