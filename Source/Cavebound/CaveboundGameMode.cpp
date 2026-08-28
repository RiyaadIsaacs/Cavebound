#include "CaveboundGameMode.h"
#include "CaveboundCharacter.h"
#include "CaveboundPlayerController.h"
#include "CaveboundTree.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ACaveboundGameMode::ACaveboundGameMode()
{
	DefaultPawnClass = ACaveboundCharacter::StaticClass();
	PlayerControllerClass = ACaveboundPlayerController::StaticClass();
}

void ACaveboundGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
}

void ACaveboundGameMode::StartPlay()
{
	Super::StartPlay();
	EnsureTree();
}

void ACaveboundGameMode::EnsureTree()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Remove the old cylinder stand-in from the Playground map, if it is still there.
	TArray<AActor*> StandIns;
	UGameplayStatics::GetAllActorsWithTag(World, FName(TEXT("PlaygroundTree")), StandIns);
	for (AActor* StandIn : StandIns)
	{
		if (StandIn)
		{
			StandIn->Destroy();
		}
	}

	TArray<AActor*> ExistingTrees;
	UGameplayStatics::GetAllActorsOfClass(World, ACaveboundTree::StaticClass(), ExistingTrees);
	if (ExistingTrees.Num() > 0)
	{
		Tree = Cast<ACaveboundTree>(ExistingTrees[0]);
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Tree = World->SpawnActor<ACaveboundTree>(
		ACaveboundTree::StaticClass(),
		FVector(500.f, 0.f, 0.f),
		FRotator::ZeroRotator,
		SpawnParams);
}

void ACaveboundGameMode::AddWood(int32 Amount)
{
	if (bGameOver || Amount <= 0)
	{
		return;
	}

	Wood += Amount;
}

void ACaveboundGameMode::HandleTreeDestroyed()
{
	bGameOver = true;
}
