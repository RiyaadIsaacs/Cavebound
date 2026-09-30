#include "CaveboundPoisonDome.h"

#include "CaveboundBaseEnemy.h"
#include "CaveboundGameMode.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundPoisonDome::ACaveboundPoisonDome()
{
	bAutoFire = false;

	VisualMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.6f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CylinderMesh.Object);
	}

	Cost = 190;
	MaxHealth = 130.f;
	Health = MaxHealth;
}

void ACaveboundPoisonDome::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsDestroyed())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		DrawDebugSphere(World, GetActorLocation(), DomeRadius, 24, FColor(70, 180, 80), false, -1.f, 0, 1.5f);
	}

	if (!IsCombatRound())
	{
		return;
	}

	UpdateDome(DeltaTime);
}

bool ACaveboundPoisonDome::IsCombatRound() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>();
	return !GameMode || GameMode->GetRoundState() == ECaveboundRoundState::Combat;
}

void ACaveboundPoisonDome::UpdateDome(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsOfClass(World, ACaveboundBaseEnemy::StaticClass(), FoundEnemies);

	TArray<TWeakObjectPtr<ACaveboundBaseEnemy>> Inside;
	const FVector Origin = GetActorLocation();

	for (AActor* Actor : FoundEnemies)
	{
		ACaveboundBaseEnemy* Enemy = Cast<ACaveboundBaseEnemy>(Actor);
		if (!Enemy || Enemy->IsDead())
		{
			continue;
		}

		if (FVector::Dist2D(Origin, Enemy->GetActorLocation()) > DomeRadius)
		{
			continue;
		}

		Enemy->ApplySlow(SlowMultiplier);
		Inside.Add(Enemy);

		float& TimeRemaining = PoisonTimeRemaining.FindOrAdd(Enemy, 0.f);
		TimeRemaining -= DeltaTime;
		if (TimeRemaining <= 0.f)
		{
			Enemy->ApplyDamage(PoisonDamagePerSecond * PoisonTickInterval);
			TimeRemaining = PoisonTickInterval;
		}
	}

	for (auto It = PoisonTimeRemaining.CreateIterator(); It; ++It)
	{
		if (!Inside.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}
}
