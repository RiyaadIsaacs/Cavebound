#include "CaveboundSnare.h"

#include "CaveboundBaseEnemy.h"
#include "CaveboundGameMode.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundSnare::ACaveboundSnare()
{
	bAutoFire = false;

	VisualMesh->SetRelativeScale3D(FVector(0.9f, 0.9f, 0.25f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMesh.Object);
	}

	Cost = 140;
	MaxHealth = 100.f;
	Health = MaxHealth;
}

void ACaveboundSnare::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ACaveboundBaseEnemy* Enemy = TrappedEnemy.Get())
	{
		Enemy->ReleaseSnare(this);
	}
	TrappedEnemy = nullptr;

	Super::EndPlay(EndPlayReason);
}

void ACaveboundSnare::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsDestroyed())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		DrawDebugSphere(World, GetActorLocation(), CatchRange, 20, FColor(200, 170, 60), false, -1.f, 0, 1.f);
		if (const ACaveboundBaseEnemy* Held = TrappedEnemy.Get())
		{
			DrawDebugSphere(World, Held->GetActorLocation(), 80.f, 12, FColor(230, 90, 40), false, -1.f, 0, 2.f);
		}
	}

	if (!IsCombatRound())
	{
		return;
	}

	UpdateSnare();
}

bool ACaveboundSnare::IsCombatRound() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>();
	return !GameMode || GameMode->GetRoundState() == ECaveboundRoundState::Combat;
}

void ACaveboundSnare::UpdateSnare()
{
	if (ACaveboundBaseEnemy* Held = TrappedEnemy.Get())
	{
		if (Held->IsSnaredBy(this) && !Held->IsDead())
		{
			return;
		}

		TrappedEnemy = nullptr;
	}

	if (ACaveboundBaseEnemy* Target = FindSnareTarget())
	{
		if (Target->TrySnare(this, TrapDuration))
		{
			TrappedEnemy = Target;
		}
	}
}

ACaveboundBaseEnemy* ACaveboundSnare::FindSnareTarget() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> FoundEnemies;
	UGameplayStatics::GetAllActorsOfClass(World, ACaveboundBaseEnemy::StaticClass(), FoundEnemies);

	ACaveboundBaseEnemy* Nearest = nullptr;
	float BestDistance = CatchRange;
	const FVector Origin = GetActorLocation();

	for (AActor* Actor : FoundEnemies)
	{
		ACaveboundBaseEnemy* Enemy = Cast<ACaveboundBaseEnemy>(Actor);
		if (!Enemy || !Enemy->CanBeSnared())
		{
			continue;
		}

		const float Distance = FVector::Dist2D(Origin, Enemy->GetActorLocation());
		if (Distance <= BestDistance)
		{
			BestDistance = Distance;
			Nearest = Enemy;
		}
	}

	return Nearest;
}
