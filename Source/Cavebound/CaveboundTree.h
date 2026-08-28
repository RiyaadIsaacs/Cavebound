#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CaveboundTree.generated.h"

class UStaticMeshComponent;
class USceneComponent;

/**
 * The central tower: a magical wood tree.
 */
UCLASS()
class CAVEBOUND_API ACaveboundTree : public AActor
{
	GENERATED_BODY()

public:
	ACaveboundTree();

	// How close the player must stand to mine (cm).
	float GetMineRange() const { return MineRange; }

	// Seconds between each wood gained during mining.
	float GetHarvestInterval() const { return HarvestInterval; }

	// Wood added per harvest. 
	int32 GetWoodPerHarvest() const { return WoodPerHarvest; }

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	float GetMaxHealth() const { return MaxHealth; }

	// Called by enemies 
	void ApplyDamage(float Amount);

	bool IsDestroyed() const { return Health <= 0.f; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tree")
	TObjectPtr<USceneComponent> SceneRoot;

	// Tree trunk
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tree")
	TObjectPtr<UStaticMeshComponent> TrunkMesh;

	// Tree canopy thing
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tree")
	TObjectPtr<UStaticMeshComponent> CanopyMesh;

	UPROPERTY(EditAnywhere, Category = "Mining")
	float MineRange = 250.f;

	UPROPERTY(EditAnywhere, Category = "Mining")
	float HarvestInterval = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Mining")
	int32 WoodPerHarvest = 5;

	UPROPERTY(EditAnywhere, Category = "Tower")
	float MaxHealth = 1000.f;

	UPROPERTY(VisibleAnywhere, Category = "Tower")
	float Health = 1000.f;
};
