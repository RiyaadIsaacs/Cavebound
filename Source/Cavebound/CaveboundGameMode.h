#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CaveboundEnemyBase.h"
#include "CaveboundTree.h"
#include "CaveboundGameMode.generated.h"

class USplineComponent;

/**
 * Match rules: which Character and PlayerController to spawn,
 * add wood (the resource mined from the tree) and gameover.
 */
UCLASS()
class CAVEBOUND_API ACaveboundGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACaveboundGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	int32 GetWood() const { return Wood; }

	void AddWood(int32 Amount);

	// Called by the tree when its health reaches 0. 
	void HandleTreeDestroyed();

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	bool IsGameOver() const { return bGameOver; }

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	ACaveboundTree* GetTree() const { return Tree; }

protected:
	// Spawn the magical wood tree if the level does not already have one. 
	void EnsureTree();

	// Find Path1/Path2/Path3 
	void CollectPathSplines();

	// Spawn one enemy on the next path, cycling 1 → 2 → 3. 
	void SpawnEnemy();

	// Amount of wood the player has
	UPROPERTY(VisibleAnywhere, Category = "Resources")
	int32 Wood = 0;

	UPROPERTY(VisibleAnywhere, Category = "Game")
	bool bGameOver = false;

	UPROPERTY()
	TObjectPtr<ACaveboundTree> Tree;

	// Path1, Path2, Path3 splines on BP_ProceduralTerrain. 
	TArray<TWeakObjectPtr<USplineComponent>> PathSplines;

	// Enemy type to spawn
	UPROPERTY(EditAnywhere, Category = "Enemies")
	TSubclassOf<ACaveboundEnemyBase> EnemyClass;

	UPROPERTY(EditAnywhere, Category = "Enemies")
	float EnemySpawnInterval = 3.0f;

	// Delay for terrain generation first
	UPROPERTY(EditAnywhere, Category = "Enemies")
	float FirstEnemyDelay = 2.0f;

	// Cycle through Path1 - Path2 - Path3
	int32 NextPathIndex = 0;
	FTimerHandle EnemySpawnTimer;
};
