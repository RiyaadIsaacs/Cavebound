#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CaveboundTreeProjectile.generated.h"

class UStaticMeshComponent;
class ACaveboundBaseEnemy;

/**
 * Small cube fired by the centre tree at the nearest enemy.
 */
UCLASS()
class CAVEBOUND_API ACaveboundTreeProjectile : public AActor
{
	GENERATED_BODY()

public:
	ACaveboundTreeProjectile();

	virtual void Tick(float DeltaTime) override;

	void Init(ACaveboundBaseEnemy* InTarget, float InDamage, float InSpeed);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnMeshBeginOverlap(UPrimitiveComponent* OverlappedComponent,AActor* OtherActor,UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void TryHitEnemy(AActor* OtherActor);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	TWeakObjectPtr<ACaveboundBaseEnemy> Target;

	float Damage = 10.f;
	float Speed = 900.f;
	float Lifetime = 0.f;
	float MaxLifetime = 3.f;
};
