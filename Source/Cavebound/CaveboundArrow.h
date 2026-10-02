#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CaveboundArrow.generated.h"

class UStaticMeshComponent;
class ACaveboundBaseEnemy;

//Projectile
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

	UPROPERTY(EditAnywhere, Category = "Combat")
	float HitRadius = 140.f;

	// Target enemy
	TWeakObjectPtr<ACaveboundBaseEnemy> Target;

	// Air duration
	float Lifetime = 0.f;

	// Prevents damaging multiple enemies
	bool bHasHit = false;
};
