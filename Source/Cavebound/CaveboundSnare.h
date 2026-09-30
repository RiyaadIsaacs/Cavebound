#pragma once

#include "CoreMinimal.h"
#include "CaveboundTurret.h"
#include "CaveboundSnare.generated.h"

class ACaveboundBaseEnemy;

/**
 * Catches one enemy at a time and holds them still. That enemy is never caught again.
 */
UCLASS(Blueprintable)
class CAVEBOUND_API ACaveboundSnare : public ACaveboundTurret
{
	GENERATED_BODY()

public:
	ACaveboundSnare();

	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Snare")
	float CatchRange = 450.f;

	UPROPERTY(EditAnywhere, Category = "Snare")
	float TrapDuration = 3.f;

	TWeakObjectPtr<ACaveboundBaseEnemy> TrappedEnemy;

	bool IsCombatRound() const;
	void UpdateSnare();
	ACaveboundBaseEnemy* FindSnareTarget() const;
};
