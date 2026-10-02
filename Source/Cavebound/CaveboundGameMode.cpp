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
#include "EngineUtils.h"
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
	bGameOver = false;
	bEndRoundPending = false;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("StartPlay: CollectionDuration=%.1fs paths=%d pool=%d"),
		CollectionDuration,
		PathSplines.Num(),
		EnemySpawnPool.Num());
}

void ACaveboundGameMode::EnsureGameplayReady()
{
	EnsureTree();
	EnsureEnemySpawnConfig();
	CollectPathSplines();
}

void ACaveboundGameMode::EnsureEnemySpawnConfig()
{
	auto IsSpawnableEnemyClass = [](const TSubclassOf<ACaveboundBaseEnemy>& Class) -> bool
	{
		return Class && !Class->HasAnyClassFlags(CLASS_Abstract);
	};

	if (!IsSpawnableEnemyClass(EnemyClass))
	{
		EnemyClass = ACaveboundEnemy::StaticClass();
	}

	// Drop null / abstract entries (Abstract base must never be spawned).
	for (int32 Index = EnemySpawnPool.Num() - 1; Index >= 0; --Index)
	{
		if (!IsSpawnableEnemyClass(EnemySpawnPool[Index].EnemyClass))
		{
			EnemySpawnPool.RemoveAt(Index);
		}
	}

	// BP_CaveboundGameMode can serialize an empty pool and wipe constructor defaults.
	if (EnemySpawnPool.Num() == 0)
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

	UE_LOG(
		LogTemp,
		Log,
		TEXT("EnsureEnemySpawnConfig: EnemyClass=%s pool=%d"),
		EnemyClass ? *EnemyClass->GetName() : TEXT("null"),
		EnemySpawnPool.Num());
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
	PathSplines.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<USplineComponent*> NamedPaths;
	TArray<USplineComponent*> AnyValid;

	auto ConsiderSpline = [&NamedPaths, &AnyValid](USplineComponent* Spline)
	{
		if (!Spline || !IsValid(Spline) || Spline->GetNumberOfSplinePoints() < 2)
		{
			return;
		}

		// Procedural paths may need a refresh before length / points are trustworthy.
		Spline->UpdateSpline();
		AnyValid.Add(Spline);

		// Prefer BP_ProceduralTerrain Path1 / Path2 / Path3, but do not require the name.
		if (Spline->GetName().Contains(TEXT("Path")))
		{
			NamedPaths.Add(Spline);
		}
	};

	auto CollectFromActor = [&ConsiderSpline](AActor* Actor)
	{
		if (!Actor)
		{
			return;
		}

		TArray<USplineComponent*> Splines;
		Actor->GetComponents<USplineComponent>(Splines);
		for (USplineComponent* Spline : Splines)
		{
			ConsiderSpline(Spline);
		}
	};

	// Prefer the procedural terrain actor — more reliable after map expansion.
	UClass* TerrainClass = LoadClass<AActor>(
		nullptr,
		TEXT("/Game/Blueprints/BP_ProceduralTerrain.BP_ProceduralTerrain_C"));
	if (TerrainClass)
	{
		TArray<AActor*> Terrains;
		UGameplayStatics::GetAllActorsOfClass(World, TerrainClass, Terrains);
		for (AActor* Terrain : Terrains)
		{
			CollectFromActor(Terrain);
		}
	}

	if (NamedPaths.Num() == 0 && AnyValid.Num() == 0)
	{
		// Fallback: scan the whole world (older levels / renamed terrain).
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), Actors);
		for (AActor* Actor : Actors)
		{
			CollectFromActor(Actor);
		}
	}

	const TArray<USplineComponent*>& Chosen =
		NamedPaths.Num() > 0 ? NamedPaths : AnyValid;
	for (USplineComponent* Spline : Chosen)
	{
		PathSplines.Add(Spline);
	}

	if (PathSplines.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("CollectPathSplines: no usable path splines found"));
	}
	else
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("CollectPathSplines: cached %d path(s) (named=%d any=%d)"),
			PathSplines.Num(),
			NamedPaths.Num(),
			AnyValid.Num());
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

	// Tree recover (cap 15% MaxHealth). If the tree was missing >=10%, ease difficulty
	if (Tree && !Tree->IsDestroyed())
	{
		const float Missing = FMath::Max(0.f, TreeMax - TreeHealthNow);
		const float MissingRatio = Missing / TreeMax;
		const float HealCap = TreeMax * FMath::Clamp(TreeHealthRecoverMaxPercent, 0.f, 1.f);
		const float Healed = Tree->Heal(FMath::Min(Missing, HealCap));

		if (MissingRatio >= TreeHealthRecoverDifficultyThreshold)
		{
			Delta -= FMath::Max(0, TreeHealthRecoverDifficultyPenalty);
		}

		UE_LOG(
			LogTemp,
			Log,
			TEXT("Tree recover: missing=%.0f%% healed=%.0f (cap %.0f%%) difficultyDelta=%d"),
			MissingRatio * 100.f,
			Healed,
			TreeHealthRecoverMaxPercent * 100.f,
			Delta);
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
		if (!Entry.EnemyClass || Entry.EnemyClass->HasAnyClassFlags(CLASS_Abstract))
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
		if (EnemyClass && !EnemyClass->HasAnyClassFlags(CLASS_Abstract))
		{
			return EnemyClass;
		}
		return ACaveboundEnemy::StaticClass();
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
	UE_LOG(
		LogTemp,
		Log,
		TEXT("StartRound called: bGameOver=%s RoundState=%d CollectionDuration=%.1fs"),
		bGameOver ? TEXT("true") : TEXT("false"),
		static_cast<int32>(RoundState),
		CollectionDuration);

	if (bGameOver)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartRound skipped: game over"));
		return;
	}

	// Combat began but nothing spawned (timer/hotreload glitch) — keep spawning.
	if (RoundState == ECaveboundRoundState::Combat
		&& EnemiesSpawnedThisRound == 0
		&& EnemiesToSpawnThisRound > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartRound: Combat with 0 spawned — forcing SpawnEnemy"));
		EnsureEnemySpawnConfig();
		CollectPathSplines();
		SpawnEnemy();
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				EnemySpawnTimer,
				this,
				&ACaveboundGameMode::SpawnEnemy,
				GetCurrentSpawnInterval(),
				true,
				GetCurrentSpawnInterval());
		}
		return;
	}

	// Stuck Collecting with no timer (hotreload / cleared handle) - recover into combat.
	if (RoundState == ECaveboundRoundState::Collecting)
	{
		bool bTimerActive = false;
		if (UWorld* World = GetWorld())
		{
			bTimerActive = World->GetTimerManager().IsTimerActive(CollectionTimer);
		}
		if (!bTimerActive)
		{
			UE_LOG(LogTemp, Warning, TEXT("StartRound: Collecting with no timer - forcing BeginCombatPhase"));
			BeginCombatPhase();
			return;
		}
		UE_LOG(LogTemp, Warning, TEXT("StartRound skipped: already collecting (timer active)"));
		return;
	}

	if (RoundState != ECaveboundRoundState::Idle)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("StartRound skipped: RoundState=%d (need Idle)"),
			static_cast<int32>(RoundState));
		return;
	}

	EnsureEnemySpawnConfig();
	ClearRoundTimers();
	EnemiesSpawnedThisRound = 0;
	EnemiesAliveThisRound = 0;
	NextPathIndex = 0;
	RoundState = ECaveboundRoundState::Collecting;

	const float GatherSeconds = FMath::Max(0.f, CollectionDuration);
	UE_LOG(
		LogTemp,
		Log,
		TEXT("StartRound: collecting for %.1fs (0 = immediate combat)"),
		GatherSeconds);

	if (GatherSeconds <= KINDA_SMALL_NUMBER)
	{
		BeginCombatPhase();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CollectionTimer,
			this,
			&ACaveboundGameMode::BeginCombatPhase,
			GatherSeconds,
			false);
	}
	else
	{
		BeginCombatPhase();
	}
}

void ACaveboundGameMode::BeginCombatPhase()
{
	UE_LOG(
		LogTemp,
		Log,
		TEXT("BeginCombatPhase called: bGameOver=%s RoundState=%d"),
		bGameOver ? TEXT("true") : TEXT("false"),
		static_cast<int32>(RoundState));

	if (bGameOver)
	{
		UE_LOG(LogTemp, Warning, TEXT("BeginCombatPhase skipped: game over"));
		return;
	}

	// Only valid from Collecting (StartRound sets that before the timer / immediate call).
	if (RoundState != ECaveboundRoundState::Collecting)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("BeginCombatPhase skipped: RoundState=%d (need Collecting)"),
			static_cast<int32>(RoundState));
		return;
	}

	RoundState = ECaveboundRoundState::Combat;
	EnsureEnemySpawnConfig();
	PrepareWaveBudget();
	CollectPathSplines();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("BeginCombatPhase: spawn %d enemies, interval=%.2fs, paths=%d, class=%s"),
		EnemiesToSpawnThisRound,
		GetCurrentSpawnInterval(),
		PathSplines.Num(),
		EnemyClass ? *EnemyClass->GetName() : TEXT("null"));

	// First enemy immediately — do not depend solely on the repeating timer first-fire.
	SpawnEnemy();

	if (UWorld* World = GetWorld())
	{
		if (EnemiesSpawnedThisRound < EnemiesToSpawnThisRound)
		{
			World->GetTimerManager().SetTimer(
				EnemySpawnTimer,
				this,
				&ACaveboundGameMode::SpawnEnemy,
				GetCurrentSpawnInterval(),
				true,
				GetCurrentSpawnInterval());
		}
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

bool ACaveboundGameMode::IsSpawnPointClear(const FVector& SpawnPoint) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}

	for (TActorIterator<ACaveboundBaseEnemy> It(World); It; ++It)
	{
		const ACaveboundBaseEnemy* Other = *It;
		if (!Other || Other->IsDead())
		{
			continue;
		}

		if (FVector::Dist2D(SpawnPoint, Other->GetActorLocation()) < 180.f)
		{
			return false;
		}
	}

	return true;
}

void ACaveboundGameMode::SpawnEnemy()
{
	if (bGameOver || RoundState != ECaveboundRoundState::Combat)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("SpawnEnemy skipped: gate bGameOver=%s RoundState=%d"),
			bGameOver ? TEXT("true") : TEXT("false"),
			static_cast<int32>(RoundState));
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

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnEnemy skipped: no World"));
		return;
	}

	EnsureTree();

	// Always refresh — weak refs can go stale after terrain expansion.
	CollectPathSplines();

	USplineComponent* ChosenSpline = nullptr;
	const int32 PathCount = PathSplines.Num();
	for (int32 Attempt = 0; Attempt < PathCount; ++Attempt)
	{
		USplineComponent* Candidate = PathSplines[(NextPathIndex + Attempt) % PathCount].Get();
		if (!Candidate || !IsValid(Candidate) || Candidate->GetNumberOfSplinePoints() < 2)
		{
			continue;
		}

		Candidate->UpdateSpline();
		// Accept any rebuilt spline with at least two points — do not reject short lengths.
		ChosenSpline = Candidate;
		NextPathIndex = (NextPathIndex + Attempt + 1) % PathCount;
		break;
	}

	TSubclassOf<ACaveboundBaseEnemy> ChosenClass = PickEnemyClassForSpawn();
	if (!ChosenClass || ChosenClass->HasAnyClassFlags(CLASS_Abstract))
	{
		ChosenClass = EnemyClass;
	}
	if (!ChosenClass || ChosenClass->HasAnyClassFlags(CLASS_Abstract))
	{
		ChosenClass = ACaveboundEnemy::StaticClass();
	}

	if (!ChosenClass)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnEnemy skipped: no spawnable enemy class"));
		return;
	}

	FVector SpawnLoc = FVector::ZeroVector;
	if (ChosenSpline)
	{
		ChosenSpline->UpdateSpline();
		const float SplineLength = ChosenSpline->GetSplineLength();
		const FVector StartLoc =
			ChosenSpline->GetLocationAtDistanceAlongSpline(0.f, ESplineCoordinateSpace::World);
		const FVector EndLoc =
			ChosenSpline->GetLocationAtDistanceAlongSpline(SplineLength, ESplineCoordinateSpace::World);
		const FVector Anchor = Tree ? Tree->GetActorLocation() : FVector::ZeroVector;
		const bool bStartIsOuter =
			FVector::DistSquared2D(StartLoc, Anchor) >= FVector::DistSquared2D(EndLoc, Anchor);
		SpawnLoc = bStartIsOuter ? StartLoc : EndLoc;
	}
	else if (Tree)
	{
		// Last resort so a missing spline never silently yields zero enemies.
		SpawnLoc = Tree->GetActorLocation() + FVector(800.f, 0.f, 50.f);
		UE_LOG(LogTemp, Warning, TEXT("SpawnEnemy: no path spline — spawning near tree at %s"), *SpawnLoc.ToCompactString());
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("SpawnEnemy skipped: spline=missing class=%s paths=%d spawned=%d/%d"),
			*ChosenClass->GetName(),
			PathCount,
			EnemiesSpawnedThisRound,
			EnemiesToSpawnThisRound);
		return;
	}

	// The entrance is full. Leave this enemy for the next timer tick.
	if (!IsSpawnPointClear(SpawnLoc))
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = this;
	ACaveboundBaseEnemy* Enemy = World->SpawnActor<ACaveboundBaseEnemy>(
		ChosenClass,
		SpawnLoc,
		FRotator::ZeroRotator,
		SpawnParams);
	if (!Enemy)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("SpawnEnemy: SpawnActor failed for %s (abstract=%s)"),
			*ChosenClass->GetName(),
			ChosenClass->HasAnyClassFlags(CLASS_Abstract) ? TEXT("yes") : TEXT("no"));
		return;
	}

	// Cycle left / centre / right so packs share the path without stacking
	const float LaneSlots[3] = { -0.7f, 0.f, 0.7f };
	const float MaxLane = Enemy->GetMaxLaneOffset();
	const float PreferredLane = LaneSlots[EnemiesSpawnedThisRound % 3] * MaxLane;

	Enemy->InitAlongPath(ChosenSpline, Tree, PreferredLane);
	++EnemiesSpawnedThisRound;
	++EnemiesAliveThisRound;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("SpawnEnemy: spawned %s on %s at %s (%d/%d alive=%d)"),
		*ChosenClass->GetName(),
		ChosenSpline ? *ChosenSpline->GetName() : TEXT("none"),
		*Enemy->GetActorLocation().ToCompactString(),
		EnemiesSpawnedThisRound,
		EnemiesToSpawnThisRound,
		EnemiesAliveThisRound);

	if (EnemiesSpawnedThisRound >= EnemiesToSpawnThisRound)
	{
		World->GetTimerManager().ClearTimer(EnemySpawnTimer);
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

// Seconds left in the collection phase. Returns 0 unless Collecting.
float ACaveboundGameMode::GetCollectionTimeRemaining() const
{
	if (RoundState != ECaveboundRoundState::Collecting)
	{
		return 0.f;
	}

	if (UWorld* World = GetWorld())
	{
		const FTimerManager& TimerManager = World->GetTimerManager();
		if (TimerManager.IsTimerActive(CollectionTimer))
		{
			return FMath::Max(0.f, TimerManager.GetTimerRemaining(CollectionTimer));
		}
	}

	// Collecting but timer not running yet (or duration was 0) — report full duration.
	return FMath::Max(0.f, CollectionDuration);
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

int32 ACaveboundGameMode::GetWoodPerHarvest() const
{
	const int32 Base = Tree ? Tree->GetWoodPerHarvest() : 5;
	const int32 BonusSteps = FMath::Max(0, RoundIndex) / 2;
	const int32 Uncapped = Base + BonusSteps * WoodBonusEveryTwoRounds;
	return FMath::Clamp(Uncapped, 1, FMath::Max(1, MaxWoodPerHarvest));
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
	if (bGameOver)
	{
		return;
	}

	bGameOver = true;
	RoundState = ECaveboundRoundState::GameOver;
	ClearRoundTimers();

	UE_LOG(LogTemp, Warning, TEXT("HandleTreeDestroyed: game over"));

	// Stop any living enemies so the match clearly ends.
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsOfClass(World, ACaveboundBaseEnemy::StaticClass(), FoundEnemies);
	for (AActor* Actor : FoundEnemies)
	{
		if (ACaveboundBaseEnemy* Enemy = Cast<ACaveboundBaseEnemy>(Actor))
		{
			if (!Enemy->IsDead())
			{
				Enemy->Destroy();
			}
		}
	}
}
