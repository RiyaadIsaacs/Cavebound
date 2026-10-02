#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CaveboundEnemyBolt.generated.h"

class UStaticMeshComponent;

/**
 * Magic shot fired by the long-range enemy.
 * It flies at a turret or the tree and hurts it on arrival.
 */
UCLASS()
class CAVEBOUND_API ACaveboundEnemyBolt : public AActor
{
	GENERATED_BODY()

public:
	ACaveboundEnemyBolt();

	virtual void Tick(float DeltaTime) override;

	// Point this bolt at the thing the mage is attacking.
	void Init(AActor* InTarget, float InDamage);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY()
	TObjectPtr<AActor> Target;

	float Damage = 12.f;
	float Speed = 900.f;
	float LifeTime = 4.f;
	float Age = 0.f;

	void HitTarget();
};
