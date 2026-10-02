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

	virtual FText GetHoverDisplayName_Implementation() const override { return FText::FromString(TEXT("Brute")); }

protected:
	virtual AActor* ResolveAttackTarget() const override;
};
