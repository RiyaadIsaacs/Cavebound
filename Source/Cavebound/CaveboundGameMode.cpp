#include "CaveboundGameMode.h"
#include "CaveboundCharacter.h"
#include "CaveboundEnemy.h"
#include "CaveboundBaseEnemy.h"
#include "CaveboundPlayerController.h"
#include "CaveboundTree.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ACaveboundGameMode::ACaveboundGameMode()
{
	DefaultPawnClass = ACaveboundCharacter::StaticClass();
	PlayerControllerClass = ACaveboundPlayerController::StaticClass();
	EnemyClass = ACaveboundEnemy::StaticClass();
}

void ACaveboundGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
}

void ACaveboundGameMode::StartPlay()
{
	Super::StartPlay();
	EnsureTree();
	CollectPathSplines();
	RoundState = ECaveboundRoundState::Idle;
}

void ACaveboundGameMode::EnsureTree()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Remove any placeholder actors with the tag PlaygroundTree
	TArray<AActor*> StandIns;
	UGameplayStatics::GetAllActorsWithTag(World, FName(TEXT("PlaygroundTree")), StandIns);
	for (AActor* StandIn : StandIns)
	{
		if (StandIn)
		{
			StandIn->Destroy();
		}
	}

	// If the level already has a tree, use it. Otherwise spawn one
	TArray<AActor*> ExistingTrees;
	UGameplayStatics::GetAllActorsOfClass(World, ACaveboundTree::StaticClass(), ExistingTrees);
	if (ExistingTrees.Num() > 0)
	{
		Tree = Cast<ACaveboundTree>(ExistingTrees[0]);
		return;
	}

	// Spawn a new tree at a fixed location
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Tree = World->SpawnActor<ACaveboundTree>(
		ACaveboundTree::StaticClass(),
		FVector(0, 0.f, 0.f),
		FRotator::ZeroRotator,
		SpawnParams);
}

void ACaveboundGameMode::CollectPathSplines()
{
	// Look up Path* splines by name
	PathSplines.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Find all actors in the world and look for USplineComponent named Path1, Path2, Path3
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), Actors);
	for (AActor* Actor : Actors)
	{
		if (!Actor)
		{
			continue;
		}

		// Get all spline components on this actor
		TArray<USplineComponent*> Splines;
		Actor->GetComponents<USplineComponent>(Splines);
		for (USplineComponent* Spline : Splines)
		{
			if (!Spline || Spline->GetNumberOfSplinePoints() < 2)
			{
				continue;
			}

			const FString SplineName = Spline->GetName();

			// BP_ProceduralTerrain names them Path1 / Path2 / Path3
			if (SplineName.Contains(TEXT("Path")))
			{
				PathSplines.Add(Spline);
			}
		}
	}
}

void ACaveboundGameMode::StartRound()
{
	if (bGameOver || RoundState != ECaveboundRoundState::Idle)
	{
		return;
	}

	ClearRoundTimers();
	EnemiesSpawnedThisRound = 0;
	EnemiesAliveThisRound = 0;
	NextPathIndex = 0;
	RoundState = ECaveboundRoundState::Collecting;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(CollectionTimer,this,&ACaveboundGameMode::BeginCombatPhase,CollectionDuration,false);
	}
}

void ACaveboundGameMode::BeginCombatPhase()
{
	if (bGameOver || RoundState != ECaveboundRoundState::Collecting)
	{
		return;
	}

	RoundState = ECaveboundRoundState::Combat;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(EnemySpawnTimer,this,&ACaveboundGameMode::SpawnEnemy,EnemySpawnInterval,true,0.f);
	}
}

void ACaveboundGameMode::EndRound()
{
	ClearRoundTimers();
	EnemiesSpawnedThisRound = 0;
	EnemiesAliveThisRound = 0;
	RoundState = ECaveboundRoundState::Idle;
}

void ACaveboundGameMode::ClearRoundTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CollectionTimer);
		World->GetTimerManager().ClearTimer(EnemySpawnTimer);
	}
}

void ACaveboundGameMode::SpawnEnemy()
{
	if (bGameOver || RoundState != ECaveboundRoundState::Combat)
	{
		return;
	}

	if (EnemiesSpawnedThisRound >= MaxEnemiesPerRound)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(EnemySpawnTimer);
		}
		return;
	}

	if (PathSplines.Num() == 0)
	{
		CollectPathSplines();
	}

	// Cycle paths so enemies do not all walk the same lane
	USplineComponent* ChosenSpline = nullptr;
	const int32 PathCount = PathSplines.Num();
	for (int32 Attempt = 0; Attempt < PathCount; ++Attempt)
	{
		USplineComponent* Candidate = PathSplines[(NextPathIndex + Attempt) % PathCount].Get();
		if (Candidate && Candidate->GetNumberOfSplinePoints() >= 2)
		{
			ChosenSpline = Candidate;
			NextPathIndex = (NextPathIndex + Attempt + 1) % PathCount;
			break;
		}
	}

	UWorld* World = GetWorld();
	if (!World || !ChosenSpline || !EnemyClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ACaveboundBaseEnemy* Enemy = World->SpawnActor<ACaveboundBaseEnemy>(
		EnemyClass,
		ChosenSpline->GetLocationAtDistanceAlongSpline(0.f, ESplineCoordinateSpace::World),
		FRotator::ZeroRotator,
		SpawnParams))
	{
		// Bind this enemy to the chosen path and the centre tree.
		Enemy->InitAlongPath(ChosenSpline, Tree);
		++EnemiesSpawnedThisRound;
		++EnemiesAliveThisRound;

		if (EnemiesSpawnedThisRound >= MaxEnemiesPerRound)
		{
			World->GetTimerManager().ClearTimer(EnemySpawnTimer);
		}
	}
}

// Called by enemies when they are destroyed to update if the round ends
void ACaveboundGameMode::RegisterEnemyDefeated()
{
	if (RoundState != ECaveboundRoundState::Combat)
	{
		return;
	}

	EnemiesAliveThisRound = FMath::Max(0, EnemiesAliveThisRound - 1);

	if (EnemiesSpawnedThisRound >= MaxEnemiesPerRound && EnemiesAliveThisRound <= 0)
	{
		EndRound();
	}
}

// Seconds left in the collection phase. Returns 0 unless Collecting 
float ACaveboundGameMode::GetCollectionTimeRemaining() const
{
	if (RoundState != ECaveboundRoundState::Collecting)
	{
		return 0.f;
	}

	if (UWorld* World = GetWorld())
	{
		return World->GetTimerManager().GetTimerRemaining(CollectionTimer);
	}

	return 0.f;
}

bool ACaveboundGameMode::CanCollectWood() const
{
	return RoundState == ECaveboundRoundState::Collecting
		|| RoundState == ECaveboundRoundState::Combat;
}

void ACaveboundGameMode::NotifyMiningBlocked()
{
	if (bHasShownRoundNotStartedMessage || CanCollectWood() || bGameOver)
	{
		return;
	}

	bHasShownRoundNotStartedMessage = true;
	bPendingRoundNotStartedPopup = true;
}

// Pop up the "Round Not Started" message once, then clear it
bool ACaveboundGameMode::ShouldShowRoundNotStartedPopup()
{
	if (!bPendingRoundNotStartedPopup)
	{
		return false;
	}

	bPendingRoundNotStartedPopup = false;
	return true;
}

void ACaveboundGameMode::AddWood(int32 Amount)
{
	if (bGameOver || Amount <= 0 || !CanCollectWood())
	{
		return;
	}

	Wood += Amount;
}

bool ACaveboundGameMode::UseWood(int32 Amount)
{
	if (bGameOver || Amount <= 0 || Wood < Amount)
	{
		return false;
	}

	Wood -= Amount;
	return true;
}

void ACaveboundGameMode::HandleTreeDestroyed()
{
	bGameOver = true;
	RoundState = ECaveboundRoundState::GameOver;
	ClearRoundTimers();
}
