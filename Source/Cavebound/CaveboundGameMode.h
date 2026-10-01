#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CaveboundBaseEnemy.h"
#include "CaveboundTree.h"
#include "CaveboundGameMode.generated.h"

class USplineComponent;

// enums for the round's state
UENUM(BlueprintType)
enum class ECaveboundRoundState : uint8
{
	Idle       UMETA(DisplayName = "Idle"),
	Collecting UMETA(DisplayName = "Collecting"),
	Combat     UMETA(DisplayName = "Combat"),
	GameOver   UMETA(DisplayName = "Game Over"),
};

USTRUCT(BlueprintType)
struct FCaveboundEnemySpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TSubclassOf<ACaveboundBaseEnemy> EnemyClass;
};

/**
 * Match rules, wood, rounds, and Adaptive Threat Budget enemy waves
 */
UCLASS()
class CAVEBOUND_API ACaveboundGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACaveboundGameMode();

	// AGameModeBase interface
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	int32 GetWood() const { return Wood; }

	void AddWood(int32 Amount);

	// Spend wood when placing a turret
	UFUNCTION(BlueprintCallable, Category = "Cavebound")
	bool UseWood(int32 Amount);

	// Called by the tree when its health reaches 0
	void HandleTreeDestroyed();

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	bool IsGameOver() const { return bGameOver; }

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	ACaveboundTree* GetTree() const { return Tree; }

	// Press Start Wave on the HUD to begin a round
	UFUNCTION(BlueprintCallable, Category = "Cavebound")
	void StartRound();

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	ECaveboundRoundState GetRoundState() const { return RoundState; }

	// check for round state
	UFUNCTION(BlueprintPure, Category = "Cavebound")
	bool IsRoundIdle() const { return RoundState == ECaveboundRoundState::Idle; }

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	bool IsRoundCollecting() const { return RoundState == ECaveboundRoundState::Collecting; }

	// Seconds left in the collection phase. Returns 0 unless Collecting
	UFUNCTION(BlueprintPure, Category = "Cavebound")
	float GetCollectionTimeRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	bool CanCollectWood() const;

	// True once after the first blocked mine attempt; clears when read
	UFUNCTION(BlueprintPure, Category = "Cavebound")
	bool ShouldShowRoundNotStartedPopup();

	void NotifyMiningBlocked();

	void RegisterEnemyDefeated();

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	int32 GetEnemiesSpawnedThisRound() const { return EnemiesSpawnedThisRound; }

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	int32 GetMaxEnemiesPerRound() const { return EnemiesToSpawnThisRound; }

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	int32 GetDifficultyScore() const { return DifficultyScore; }

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	int32 GetRoundIndex() const { return RoundIndex; }

protected:
	// Spawn the magical wood tree if the level does not already have one
	void EnsureTree();
	// Find Path1/Path2/Path3 
	void CollectPathSplines();
	// Spawn one enemy on the next path, cycling 1 - 2 - 3
	void SpawnEnemy();
	void BeginCombatPhase();
	void EndRound();
	void ClearRoundTimers();

	// Amount of wood the player has
	void PrepareWaveBudget();
	void UpdateDifficultyAfterRound();
	int32 CountLivingTurrets() const;
	TSubclassOf<ACaveboundBaseEnemy> PickEnemyClassForSpawn() const;
	float GetCurrentSpawnInterval() const;

	UPROPERTY(VisibleAnywhere, Category = "Resources")
	int32 Wood = 0;

	UPROPERTY(VisibleAnywhere, Category = "Game")
	bool bGameOver = false;

	UPROPERTY(VisibleAnywhere, Category = "Game")
	ECaveboundRoundState RoundState = ECaveboundRoundState::Idle;

	UPROPERTY()
	TObjectPtr<ACaveboundTree> Tree;

	// Path1, Path2, Path3 splines on BP_ProceduralTerrain
	TArray<TWeakObjectPtr<USplineComponent>> PathSplines;

	// Enemy type to spawn
	// Fallback if the spawn pool is empty
	UPROPERTY(EditAnywhere, Category = "Enemies")
	TSubclassOf<ACaveboundBaseEnemy> EnemyClass;

	// Basic / Brute / Long Range — filtered by DifficultyScore vs MinDifficultyToSpawn
	UPROPERTY(EditAnywhere, Category = "Enemies")
	TArray<FCaveboundEnemySpawnEntry> EnemySpawnPool;

	UPROPERTY(EditAnywhere, Category = "Enemies")
	float EnemySpawnInterval = 3.0f;

	// Round Timer
	UPROPERTY(EditAnywhere, Category = "Enemies")
	float MinSpawnInterval = 1.25f;

	UPROPERTY(EditAnywhere, Category = "Enemies")
	float MaxSpawnInterval = 3.5f;

	UPROPERTY(EditAnywhere, Category = "Round")
	float CollectionDuration = 60.0f;

	// Used as the base count before DifficultyScore / round bonuses
	UPROPERTY(EditAnywhere, Category = "Round")
	int32 MaxEnemiesPerRound = 8;

	UPROPERTY(EditAnywhere, Category = "Difficulty")
	int32 DifficultyScore = 30;

	UPROPERTY(EditAnywhere, Category = "Difficulty")
	int32 MinDifficultyScore = 0;

	UPROPERTY(EditAnywhere, Category = "Difficulty")
	int32 MaxDifficultyScore = 100;

	UPROPERTY(VisibleAnywhere, Category = "Difficulty")
	int32 RoundIndex = 0;

	int32 EnemiesSpawnedThisRound = 0;
	int32 EnemiesAliveThisRound = 0;
	int32 EnemiesToSpawnThisRound = 8;
	int32 NextPathIndex = 0;
	int32 TurretsAtCombatStart = 0;
	float TreeHealthAtCombatStart = 0.f;
	float CombatStartTimeSeconds = 0.f;

	bool bHasShownRoundNotStartedMessage = false;
	bool bPendingRoundNotStartedPopup = false;

	FTimerHandle CollectionTimer;
	FTimerHandle EnemySpawnTimer;
};
