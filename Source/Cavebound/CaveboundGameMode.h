#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CaveboundGameMode.generated.h"

/**
 * Match rules: which Character and which PlayerController to spawn.
 */
UCLASS()
class CAVEBOUND_API ACaveboundGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACaveboundGameMode();

	// Called once when the map loads, before the player is placed.
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

};
