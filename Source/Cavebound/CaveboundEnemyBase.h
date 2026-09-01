#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CaveboundEnemyBase.generated.h"

class UStaticMeshComponent;
class USplineComponent;
class ACaveboundTree;

/**
 * Base enemy with different virtual functions for different behaviour overrides
 */
UCLASS(Abstract, Blueprintable)
class CAVEBOUND_API ACaveboundBaseEnemy : public AActor
{
	GENERATED_BODY()

public:
	ACaveboundBaseEnemy();

	virtual void Tick(float DeltaTime) override;

	// Which path to walk and target to attack 
	void InitAlongPath(USplineComponent* Spline, ACaveboundTree* InTree);

	UFUNCTION(BlueprintCallable, Category = "Cavebound")
	virtual void ApplyDamage(float Amount);

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	float GetMaxHealth() const { return MaxHealth; }

	bool IsDead() const { return Health <= 0.f; }

protected:
	// Walk along PathSpline. Possible flying / jumping types later
	virtual void MoveAlongPath(float DeltaTime);

	// Damage the tree on a timer
	virtual void AttackCurrentTarget(float DeltaTime);

	// Destroy() and maybe other VFX / sounds / particles
	virtual void OnDeath();

	bool IsInAttackRange() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float MaxHealth = 40.f;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	float Health = 40.f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float MoveSpeed = 280.f;

	// Extra height so the mesh sits on the path instead of clipping through it
	UPROPERTY(EditAnywhere, Category = "Movement")
	float PathHeightOffset = 50.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackRange = 280.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackInterval = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackDamage = 8.f;

	TWeakObjectPtr<USplineComponent> PathSpline;
	TWeakObjectPtr<ACaveboundTree> Tree;

	// How far along the spline we have walked (cm)
	float DistanceAlongSpline = 0.f;

	// Time since last attack pulse
	float AttackTime = 0.f;
};
