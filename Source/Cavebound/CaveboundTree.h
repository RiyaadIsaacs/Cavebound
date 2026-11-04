#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CaveboundDamageFlash.h"
#include "CaveboundHoverHealth.h"
#include "CaveboundTree.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class ACaveboundBaseEnemy;
class ACaveboundTreeProjectile;
class ACaveboundHoverEmbers;

/**
 * The central tower: a magical wood tree.
 */
UCLASS()
class CAVEBOUND_API ACaveboundTree : public AActor, public ICaveboundHoverHealth
{
	GENERATED_BODY()

public:
	ACaveboundTree();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual float GetHoverHealth_Implementation() const override { return Health; }
	virtual float GetHoverMaxHealth_Implementation() const override { return MaxHealth; }
	virtual FText GetHoverDisplayName_Implementation() const override { return FText::FromString(TEXT("Tree")); }

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
	UFUNCTION(BlueprintCallable, Category = "Cavebound")
	void ApplyDamage(float Amount);

	// Restore HP after a cleared round. Returns amount actually healed
	UFUNCTION(BlueprintCallable, Category = "Cavebound")
	float Heal(float Amount);

	bool IsDestroyed() const { return Health <= 0.f; }

protected:
	void NotifyDestroyedByDamage();
	void PlayDamageFlash();
	void TryFireAtNearestEnemy();
	float GetCanopyHeight() const;

	ACaveboundBaseEnemy* FindNearestEnemy() const;

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

	UPROPERTY(EditAnywhere, Category = "Tower")
	float HitFlashDuration = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Tower")
	FLinearColor HitFlashColor = FLinearColor(1.f, 0.f, 0.f);

	FTimerHandle HitFlashTimer;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackRange = 1200.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float FireInterval = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ProjectileDamage = 10.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ProjectileSpeed = 900.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TSubclassOf<ACaveboundTreeProjectile> ProjectileClass;

	float FireTime = 0.f;

	UPROPERTY()
	TObjectPtr<ACaveboundHoverEmbers> GreenEmbers;
};
