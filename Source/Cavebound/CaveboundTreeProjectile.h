#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CaveboundTreeProjectile.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class ACaveboundBaseEnemy;

/** Green orb fired by the centre tree at the nearest enemy. */
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
	void OnOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void HitEnemy(AActor* OtherActor);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> Mat;

	TWeakObjectPtr<ACaveboundBaseEnemy> Target;

	float Damage = 10.f;
	float Speed = 900.f;
	float Time = 0.f;
	float MaxLife = 3.f;
};
