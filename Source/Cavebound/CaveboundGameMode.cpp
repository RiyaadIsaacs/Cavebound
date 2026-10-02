#include "CaveboundGameMode.h"
#include "CaveboundBruteEnemy.h"
#include "CaveboundCharacter.h"
#include "CaveboundEnemy.h"
#include "CaveboundBaseEnemy.h"
#include "CaveboundLongRangeEnemy.h"
#include "CaveboundPlayerController.h"
#include "CaveboundTerrainExpansion.h"
#include "CaveboundTree.h"
#include "CaveboundTurret.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ACaveboundGameMode::ACaveboundGameMode()
{
	DefaultPawnClass = ACaveboundCharacter::StaticClass();
	PlayerControllerClass = ACaveboundPlayerController::StaticClass();
	EnemyClass = ACaveboundEnemy::StaticClass();

	EnemySpawnPool.Reset();
	{
		FCaveboundEnemySpawnEntry Basic;
		Basic.EnemyClass = ACaveboundEnemy::StaticClass();
		EnemySpawnPool.Add(Basic);

		FCaveboundEnemySpawnEntry Brute;
		Brute.EnemyClass = ACaveboundBruteEnemy::StaticClass();
		EnemySpawnPool.Add(Brute);

		FCaveboundEnemySpawnEntry LongRange;
		LongRange.EnemyClass = ACaveboundLongRangeEnemy::StaticClass();
		EnemySpawnPool.Add(LongRange);
	}
}

void ACaveboundGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
}

void ACaveboundGameMode::StartPlay()
{
	Super::StartPlay();
	EnsureGameplayReady();
	RoundState = ECaveboundRoundState::Idle;
}

void ACaveboundGameMode::EnsureGameplayReady()
{
	EnsureTree();
	if (PathSplines.Num() == 0)
	{
		CollectPathSplines();
	}
}

ACaveboundTree* ACaveboundGameMode::GetTree() const
{
	if (!Tree)
	{
		const_cast<ACaveboundGameMode*>(this)->EnsureTree();
	}
	return Tree;
}

float ACaveboundGameMode::GetTreeHealthPercent() const
{
	const ACaveboundTree* CurrentTree = GetTree();
	if (!CurrentTree)
	{
		return 1.f;
	}

	const float MaxHealth = CurrentTree->GetMaxHealth();
	if (MaxHealth <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}

	return FMath::Clamp(CurrentTree->GetHealth() / MaxHealth, 0.f, 1.f);
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

			// BP_ProceduralTerrain names them Path1 / Path2 / Path3
			if (Spline->GetName().Contains(TEXT("Path")))
			{
				PathSplines.Add(Spline);
			}
		}
	}
}

int32 ACaveboundGameMode::CountLivingTurrets() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(World, ACaveboundTurret::StaticClass(), Found);

	int32 Count = 0;
	for (AActor* Actor : Found)
	{
		if (const ACaveboundTurret* Turret = Cast<ACaveboundTurret>(Actor))
		{
			if (!Turret->IsDestroyed())
			{
				++Count;
			}
		}
	}
	return Count;
}

float ACaveboundGameMode::GetCurrentSpawnInterval() const
{
	const float Alpha = FMath::Clamp(DifficultyScore / 100.f, 0.f, 1.f);
	return FMath::Lerp(MaxSpawnInterval, MinSpawnInterval, Alpha);
}

void ACaveboundGameMode::PrepareWaveBudget()
{
	TurretsAtCombatStart = CountLivingTurrets();
	TreeHealthAtCombatStart = Tree ? Tree->GetHealth() : 0.f;
	CombatStartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	const int32 RoundBonus = RoundIndex * 2;
	const int32 ScoreBonus = DifficultyScore / 12;
	EnemiesToSpawnThisRound = FMath::Max(4, MaxEnemiesPerRound + RoundBonus + ScoreBonus);
}

void ACaveboundGameMode::UpdateDifficultyAfterRound()
{
	UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : CombatStartTimeSeconds;
	const float ClearTime = FMath::Max(0.f, Now - CombatStartTimeSeconds);

	const float TreeHealthNow = Tree ? Tree->GetHealth() : 0.f;
	const float TreeDamageTaken = FMath::Max(0.f, TreeHealthAtCombatStart - TreeHealthNow);
	const float TreeMax = Tree ? FMath::Max(1.f, Tree->GetMaxHealth()) : 1.f;
	const float DamageRatio = TreeDamageTaken / TreeMax;

	int32 Delta = 5; // base climb per cleared round

	if (DamageRatio < 0.08f)
	{
		Delta += 6;
	}
	else if (DamageRatio < 0.2f)
	{
		Delta += 3;
	}
	else if (DamageRatio > 0.45f)
	{
		Delta -= 8;
	}
	else if (DamageRatio > 0.3f)
	{
		Delta -= 4;
	}

	// Fast clear = skilled; slow clear = struggling
	if (ClearTime > 0.f && ClearTime < 35.f)
	{
		Delta += 4;
	}
	else if (ClearTime > 70.f)
	{
		Delta -= 5;
	}

	// Play style: invested defences climb faster so Brutes / Long Range unlock sooner
	if (TurretsAtCombatStart >= 3)
	{
		Delta += 3;
	}
	else if (TurretsAtCombatStart == 0)
	{
		Delta -= 2;
	}

	DifficultyScore = FMath::Clamp(DifficultyScore + Delta, MinDifficultyScore, MaxDifficultyScore);
	++RoundIndex;
}

TSubclassOf<ACaveboundBaseEnemy> ACaveboundGameMode::PickEnemyClassForSpawn() const
{
	TArray<TSubclassOf<ACaveboundBaseEnemy>> Eligible;
	Eligible.Reserve(EnemySpawnPool.Num());

	for (const FCaveboundEnemySpawnEntry& Entry : EnemySpawnPool)
	{
		if (!Entry.EnemyClass)
		{
			continue;
		}

		const ACaveboundBaseEnemy* CDO = Entry.EnemyClass->GetDefaultObject<ACaveboundBaseEnemy>();
		if (!CDO)
		{
			continue;
		}

		if (DifficultyScore >= CDO->GetMinDifficultyToSpawn())
		{
			Eligible.Add(Entry.EnemyClass);
		}
	}

	if (Eligible.Num() == 0)
	{
		return EnemyClass;
	}

	// Light play-style bias: with several turrets, prefer Brute when it is eligible
	if (TurretsAtCombatStart >= 2)
	{
		for (const TSubclassOf<ACaveboundBaseEnemy>& Class : Eligible)
		{
			if (Class && Class->IsChildOf(ACaveboundBruteEnemy::StaticClass()))
			{
				if (FMath::FRand() < 0.45f)
				{
					return Class;
				}
				break;
			}
		}
	}

	const int32 Index = FMath::RandRange(0, Eligible.Num() - 1);
	return Eligible[Index];
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
		World->GetTimerManager().SetTimer(
			CollectionTimer,
			this,
			&ACaveboundGameMode::BeginCombatPhase,
			CollectionDuration,
			false);
	}
}

void ACaveboundGameMode::BeginCombatPhase()
{
	if (bGameOver || RoundState != ECaveboundRoundState::Collecting)
	{
		return;
	}

	RoundState = ECaveboundRoundState::Combat;
	PrepareWaveBudget();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			EnemySpawnTimer,
			this,
			&ACaveboundGameMode::SpawnEnemy,
			GetCurrentSpawnInterval(),
			true,
			0.f);
	}
}

void ACaveboundGameMode::EndRound()
{
	bEndRoundPending = false;

	UpdateDifficultyAfterRound();

	ClearRoundTimers();
	EnemiesSpawnedThisRound = 0;
	EnemiesAliveThisRound = 0;
	EnemiesToSpawnThisRound = MaxEnemiesPerRound;
	RoundState = ECaveboundRoundState::Idle;

	// Expand after leaving Combat and off the enemy death call stack
	if (bExpandBuildSlotsAfterRound && DifficultyScore >= BuildSlotExpandMinDifficulty)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateUObject(this, &ACaveboundGameMode::ExpandBuildSlotsNextTick));
		}
	}
}

void ACaveboundGameMode::RequestEndRound()
{
	if (bEndRoundPending || RoundState != ECaveboundRoundState::Combat)
	{
		return;
	}

	bEndRoundPending = true;

	if (UWorld* World = GetWorld())
	{
		// Next tick: enemy OnDeath / ApplyDamage / projectile overlap have finished
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &ACaveboundGameMode::EndRound));
	}
	else
	{
		EndRound();
	}
}

void ACaveboundGameMode::ExpandBuildSlotsNextTick()
{
	ExpandBuildSlots();
}

int32 ACaveboundGameMode::ExpandBuildSlots()
{
	if (bGameOver || !GetWorld())
	{
		return 0;
	}

	const int32 Spawned = FCaveboundTerrainExpansion::ExpandBuildSlots(
		GetWorld(),
		BuildSlotExpandCells,
		BuildSlotSpacingMultiplier,
		BuildSlotSpacingFloor,
		BuildSlotMaxGridSize,
		BuildSlotPathSidePadding,
		BuildSlotMaxNewPerExpand);

	// Paths were extended with the map — refresh cached splines for the next wave
	CollectPathSplines();
	return Spawned;
}

void ACaveboundGameMode::ClearRoundTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CollectionTimer);
		World->GetTimerManager().ClearTimer(EnemySpawnTimer);
		World->GetTimerManager().ClearTimer(EndRoundTimer);
		World->GetTimerManager().ClearTimer(ExpandSlotsTimer);
	}
}

void ACaveboundGameMode::SpawnEnemy()
{
	if (bGameOver || RoundState != ECaveboundRoundState::Combat)
	{
		return;
	}

	if (EnemiesSpawnedThisRound >= EnemiesToSpawnThisRound)
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
	const TSubclassOf<ACaveboundBaseEnemy> ChosenClass = PickEnemyClassForSpawn();
	if (!World || !ChosenSpline || !ChosenClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ACaveboundBaseEnemy* Enemy = World->SpawnActor<ACaveboundBaseEnemy>(
		ChosenClass,
		ChosenSpline->GetLocationAtDistanceAlongSpline(0.f, ESplineCoordinateSpace::World),
		FRotator::ZeroRotator,
		SpawnParams))
	{
		// Bind this enemy to the chosen path and the centre tree.
		Enemy->InitAlongPath(ChosenSpline, Tree);
		++EnemiesSpawnedThisRound;
		++EnemiesAliveThisRound;

		if (EnemiesSpawnedThisRound >= EnemiesToSpawnThisRound)
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

	if (EnemiesSpawnedThisRound >= EnemiesToSpawnThisRound && EnemiesAliveThisRound <= 0)
	{
		RequestEndRound();
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
