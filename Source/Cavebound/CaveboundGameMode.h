#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CaveboundTree.h"
#include "CaveboundGameMode.generated.h"

/**
 * Match rules: which Character and PlayerController to spawn,
 * plus wood (the resource mined from the tree) and game-over.
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

	/** Called by the tree when its health reaches 0. */
	void HandleTreeDestroyed();

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	bool IsGameOver() const { return bGameOver; }

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	ACaveboundTree* GetTree() const { return Tree; }

protected:
	// Spawn the magical wood tree if the level does not already have one. 
	void EnsureTree();

	UPROPERTY(VisibleAnywhere, Category = "Resources")
	int32 Wood = 0;

	UPROPERTY(VisibleAnywhere, Category = "Game")
	bool bGameOver = false;

	UPROPERTY()
	TObjectPtr<ACaveboundTree> Tree;
};
