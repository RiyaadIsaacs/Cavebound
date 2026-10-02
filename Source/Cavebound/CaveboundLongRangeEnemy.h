#pragma once

#include "CoreMinimal.h"
#include "CaveboundEnemy.h"
#include "CaveboundLongRangeEnemy.generated.h"

/**
 * Artillery enemy: stops farther out and attacks with a long AttackRange.
 */
UCLASS()
class CAVEBOUND_API ACaveboundLongRangeEnemy : public ACaveboundEnemy
{
	GENERATED_BODY()

public:
	ACaveboundLongRangeEnemy();

	virtual FText GetHoverDisplayName_Implementation() const override { return FText::FromString(TEXT("Ranger")); }

protected:
	virtual bool AttackAppliesDamageNow() const override;
	virtual void OnMeleeStrike() override;

	void FireBolt(AActor* Target);
};
