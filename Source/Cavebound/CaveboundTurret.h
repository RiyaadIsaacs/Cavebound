#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CaveboundDamageFlash.h"
#include "CaveboundHoverHealth.h"
#include "CaveboundTurret.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/**
 * Placeable defender the enemies will attack
 */
UCLASS(Blueprintable)
class CAVEBOUND_API ACaveboundTurret : public AActor, public ICaveboundHoverHealth
{
	GENERATED_BODY()

public:
	ACaveboundTurret();

	virtual void BeginPlay() override;

	// HUD popup bar on mouse hover
	virtual float GetHoverHealth_Implementation() const override { return Health; }
	virtual float GetHoverMaxHealth_Implementation() const override { return MaxHealth; }

	UFUNCTION(BlueprintCallable, Category = "Cavebound")
	void ApplyDamage(float Amount);

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	float GetMaxHealth() const { return MaxHealth; }

	bool IsDestroyed() const { return Health <= 0.f; }

protected:
	void PlayDamageFlash();
	virtual void OnDestroyedByDamage();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret")
	TObjectPtr<USceneComponent> SceneRoot;

	// Placeholder mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MaxHealth = 150.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	float Health = 150.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float HitFlashDuration = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	FLinearColor HitFlashColor = FLinearColor(1.f, 0.f, 0.f);

	FTimerHandle HitFlashTimer;
};
