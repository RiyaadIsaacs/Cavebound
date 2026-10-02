#pragma once

#include "CoreMinimal.h"
#include "CaveboundTurret.h"
#include "CaveboundPoisonDome.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class ACaveboundBaseEnemy;

// Poison and slow defender
UCLASS(Blueprintable)
class CAVEBOUND_API ACaveboundPoisonDome : public ACaveboundTurret
{
	GENERATED_BODY()

public:
	ACaveboundPoisonDome();

	virtual FText GetHoverDisplayName_Implementation() const override { return FText::FromString(TEXT("Poison Dome")); }

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	// Range
	UPROPERTY(EditAnywhere, Category = "Dome")
	float DomeRadius = 550.f;

	// Enemy slow multiplier
	UPROPERTY(EditAnywhere, Category = "Dome")
	float SlowMultiplier = 0.5f;

	// DPS
	UPROPERTY(EditAnywhere, Category = "Dome")
	float PoisonDamagePerSecond = 8.f;

	// Posion ticks
	UPROPERTY(EditAnywhere, Category = "Dome")
	float PoisonTickInterval = 0.5f;

	// Time left before each enemy inside takes the next poison hit.
	TMap<TWeakObjectPtr<ACaveboundBaseEnemy>, float> PoisonTimeRemaining;

	// Dome visual
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dome")
	TObjectPtr<UStaticMeshComponent> PoisonCloud;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> GreenMaterial;

	virtual void PlayDamageFlash() override;

	bool IsCombatRound() const;
	void UpdateDome(float DeltaTime);
	void UpdatePoisonCloud();
};
