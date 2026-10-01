#pragma once

#include "CoreMinimal.h"
#include "CaveboundTurret.h"
#include "CaveboundSnare.generated.h"

class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;
class UMaterialInstanceDynamic;
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
	// How close an enemy must be before this trap can grab them.
	UPROPERTY(EditAnywhere, Category = "Snare")
	float CatchRange = 450.f;

	// How long the grab lasts, in seconds.
	UPROPERTY(EditAnywhere, Category = "Snare")
	float TrapDuration = 3.f;

	// The enemy this trap is holding right now. Empty when the trap is free.
	TWeakObjectPtr<ACaveboundBaseEnemy> TrappedEnemy;

	// How tall the post is. The rope starts at the top.
	float PostHeight = 700.f;

	// The path the rope follows.
	UPROPERTY()
	TObjectPtr<USplineComponent> RopeSpline;

	// One thin piece of rope for each step along the path.
	UPROPERTY()
	TArray<TObjectPtr<USplineMeshComponent>> RopePieces;

	UPROPERTY()
	TObjectPtr<UStaticMesh> RopeMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> RopeMaterial;

	bool IsCombatRound() const;
	void UpdateSnare();
	ACaveboundBaseEnemy* FindSnareTarget() const;
	void ShowRope(ACaveboundBaseEnemy* CaughtEnemy);
	void ClearRope();
};
