#pragma once

#include "CoreMinimal.h"
#include "CaveboundTurret.h"
#include "CaveboundPoisonDome.generated.h"

class ACaveboundBaseEnemy;

/**
 * Holds a dome around itself. Enemies inside move slower and take poison damage.
 */
UCLASS(Blueprintable)
class CAVEBOUND_API ACaveboundPoisonDome : public ACaveboundTurret
{
	GENERATED_BODY()

public:
	ACaveboundPoisonDome();

	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Dome")
	float DomeRadius = 550.f;

	// 0.5 means enemies move at half speed while inside.
	UPROPERTY(EditAnywhere, Category = "Dome")
	float SlowMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Dome")
	float PoisonDamagePerSecond = 8.f;

	UPROPERTY(EditAnywhere, Category = "Dome")
	float PoisonTickInterval = 0.5f;

	TMap<TWeakObjectPtr<ACaveboundBaseEnemy>, float> PoisonTimeRemaining;

	bool IsCombatRound() const;
	void UpdateDome(float DeltaTime);
};
