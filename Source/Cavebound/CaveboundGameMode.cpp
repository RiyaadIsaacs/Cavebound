#include "CaveboundGameMode.h"
#include "CaveboundCharacter.h"
#include "CaveboundPlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

ACaveboundGameMode::ACaveboundGameMode()
{
	// These are the C++ fallbacks; input assets on BP_CaveboundPlayerController are used.
	DefaultPawnClass = ACaveboundCharacter::StaticClass();
	PlayerControllerClass = ACaveboundPlayerController::StaticClass();
}

void ACaveboundGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
}

