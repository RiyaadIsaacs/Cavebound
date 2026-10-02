#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CaveboundArrow.generated.h"

class UStaticMeshComponent;
class ACaveboundBaseEnemy;

/**
 * Turret projectile. Homes toward an enemy (same idea as the tree cube) and applies damage on hit.
 * Reparent BP_Arrow to this if you want the ballista mesh with reliable damage.
 */
UCLASS(Blueprintable)
class CAVEBOUND_API ACaveboundArrow : public AActor
{
	GENERATED_BODY()

public:
	ACaveboundArrow();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Cavebound")
	void Init(ACaveboundBaseEnemy* InTarget, float InDamage, float InSpeed);

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	float GetDamage() const { return Damage; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnMeshBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void TryHitEnemy(AActor* OtherActor);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float Speed = 1200.f;

	// The shot deletes itself after this many seconds.
	UPROPERTY(EditAnywhere, Category = "Combat")
	float MaxLifetime = 4.f;

	// Close enough to count as a hit even before the overlap fires.
	// Sized for enemy root (feet) while shots aim near chest height.
	UPROPERTY(EditAnywhere, Category = "Combat")
	float HitRadius = 140.f;

	// The enemy this shot is chasing.
	TWeakObjectPtr<ACaveboundBaseEnemy> Target;

	// How long this shot has been in the air.
	float Lifetime = 0.f;

	// Stops the shot from damaging more than one enemy.
	bool bHasHit = false;
};
