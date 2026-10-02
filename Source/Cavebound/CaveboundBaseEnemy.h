#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CaveboundDamageFlash.h"
#include "CaveboundHoverHealth.h"
#include "CaveboundBaseEnemy.generated.h"

class UStaticMeshComponent;
class USplineComponent;
class ACaveboundTree;
class ACaveboundTurret;

/**
 * Base enemy with different virtual functions for different behaviour overrides
 * and path-constrained boids so groups do not stack on the spline.
 */
UCLASS(Abstract, Blueprintable)
class CAVEBOUND_API ACaveboundBaseEnemy : public AActor, public ICaveboundHoverHealth
{
	GENERATED_BODY()

public:
	ACaveboundBaseEnemy();

	virtual void Tick(float DeltaTime) override;

	// ICaveboundHoverHealth interface for the HUD to show HP on mouse hover
	virtual float GetHoverHealth_Implementation() const override { return Health; }
	virtual float GetHoverMaxHealth_Implementation() const override { return MaxHealth; }

	// Which path to walk and target to attack 
	void InitAlongPath(USplineComponent* Spline, ACaveboundTree* InTree, float InPreferredLaneOffset = 0.f);

	UFUNCTION(BlueprintCallable, Category = "Cavebound")
	virtual void ApplyDamage(float Amount);

	// Slow while standing in a poison dome. 1 is full speed. Reapplied every frame the enemy stays inside.
	void ApplySlow(float SpeedMultiplier);
	float GetMoveSpeedScale() const;

	// A snare may hold this enemy once. While held, they cannot move or attack.
	bool CanBeSnared() const;
	bool TrySnare(AActor* Source, float Duration);
	void ReleaseSnare(AActor* Source);
	bool IsSnared() const { return bIsSnared; }
	bool IsSnaredBy(const AActor* Source) const;

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	int32 GetMinDifficultyToSpawn() const { return MinDifficultyToSpawn; }

	float GetDistanceAlongSpline() const { return DistanceAlongSpline; }
	float GetMaxLaneOffset() const { return MaxLaneOffset; }

	bool IsDead() const { return Health <= 0.f; }

protected:
	// Walk along PathSpline
	virtual void MoveAlongPath(float DeltaTime);
	// Damage the current turret or tree on a timer
	virtual void AttackCurrentTarget(float DeltaTime);

	// False while a hit reaction should stop this enemy from swinging
	virtual bool CanAttack() const { return true; }

	// Called once on each successful melee swing, before damage is applied
	virtual void OnMeleeStrike() {}

	// Destroy() and maybe other VFX / sounds / particles
	virtual void OnDeath();

	// Prefer turrets in detect range, else the tree when in attack range. Brute overrides.
	virtual AActor* ResolveAttackTarget() const;

	ACaveboundTurret* FindNearestTurretInRange(float Range) const;
	bool IsInAttackRangeOf(const AActor* Target) const;
	void PlayDamageFlash();

	// Separation + light alignment/cohesion projected onto the path right vector
	float ComputeBoidLaneOffset(const FVector& PathPoint, const FVector& PathRight) const;
	float ComputeForwardStagger() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	// Spawner only picks this type when DifficultyScore >= this value
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn")
	int32 MinDifficultyToSpawn = 0;

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

	// How far away a turret can be before enemies target them
	UPROPERTY(EditAnywhere, Category = "Combat")
	float TurretDetectRange = 750.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackInterval = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackDamage = 8.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float HitFlashDuration = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	FLinearColor HitFlashColor = FLinearColor(1.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, Category = "Boids")
	bool bUseBoids = true;

	// How far sideways neighbours must be before they stop pushing each other
	UPROPERTY(EditAnywhere, Category = "Boids")
	float SeparationRadius = 220.f;

	UPROPERTY(EditAnywhere, Category = "Boids")
	float SeparationStrength = 1.35f;

	// Keep low — cohesion pulls packs back onto the same spot on the spline
	UPROPERTY(EditAnywhere, Category = "Boids")
	float AlignmentStrength = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Boids")
	float CohesionStrength = 0.f;

	// Max lateral offset from the path centre (cm)
	UPROPERTY(EditAnywhere, Category = "Boids")
	float MaxLaneOffset = 200.f;

	// Preferred distance between enemies along the same path (cm)
	UPROPERTY(EditAnywhere, Category = "Boids")
	float ForwardStaggerDistance = 160.f;

	UPROPERTY(EditAnywhere, Category = "Boids")
	float ForwardStaggerStrength = 0.65f;

	FTimerHandle HitFlashTimer;

	TWeakObjectPtr<USplineComponent> PathSpline;
	TWeakObjectPtr<ACaveboundTree> Tree;

	// How far along the spline we have walked (cm)
	float DistanceAlongSpline = 0.f;
	// Stable side-of-path slot assigned at spawn (-Max..+Max)
	float PreferredLaneOffset = 0.f;
	// Time since last attack pulse
	float AttackTime = 0.f;
	float ActiveSlowMultiplier = 1.f;
	float SlowRefreshTime = -1.f;

	bool bIsSnared = false;
	bool bHasBeenSnared = false;
	TWeakObjectPtr<AActor> SnareSource;
	float SnareTimeRemaining = 0.f;
};
