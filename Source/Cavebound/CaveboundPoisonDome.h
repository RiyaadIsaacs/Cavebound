#pragma once

#include "CoreMinimal.h"
#include "CaveboundTurret.h"
#include "CaveboundPoisonDome.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class ACaveboundBaseEnemy;

/**
 * Holds a green cloud around itself. Enemies inside move slower and take poison damage.
 */
UCLASS(Blueprintable)
class CAVEBOUND_API ACaveboundPoisonDome : public ACaveboundTurret
{
	GENERATED_BODY()

public:
	ACaveboundPoisonDome();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	// How far the slow and poison reach from the defender.
	UPROPERTY(EditAnywhere, Category = "Dome")
	float DomeRadius = 550.f;

	// 0.5 means enemies move at half speed while inside.
	UPROPERTY(EditAnywhere, Category = "Dome")
	float SlowMultiplier = 0.5f;

	// Damage per second. A tick applies this times the wait below.
	UPROPERTY(EditAnywhere, Category = "Dome")
	float PoisonDamagePerSecond = 8.f;

	// Seconds between poison hits on the same enemy.
	UPROPERTY(EditAnywhere, Category = "Dome")
	float PoisonTickInterval = 0.5f;

	// Time left before each enemy inside takes the next poison hit.
	TMap<TWeakObjectPtr<ACaveboundBaseEnemy>, float> PoisonTimeRemaining;

	// The see-through green sphere. It shows how far the poison reaches.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dome")
	TObjectPtr<UStaticMeshComponent> PoisonCloud;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> GreenMaterial;

	virtual void PlayDamageFlash() override;

	bool IsCombatRound() const;
	void UpdateDome(float DeltaTime);
	void UpdatePoisonCloud();
};
