#include "CaveboundRockDeposit.h"

#include "CaveboundGameMode.h"
#include "CaveboundHarvestBar.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 RocksPerHarvest = 5;
}

ACaveboundRockDeposit::ACaveboundRockDeposit()
{
	PrimaryActorTick.bCanEverTick = false;

	// Keep the actor root fixed. Moving the mesh itself would drag the whole pile back to the origin.
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	VisualMesh->SetCollisionObjectType(ECC_WorldDynamic);
	VisualMesh->SetCollisionResponseToAllChannels(ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> stoneMesh(
		TEXT("/Game/Assets/Models/Stones/Stone.Stone"));
	if (stoneMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(stoneMesh.Object);
	}
	else
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> fallbackCube(
			TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (fallbackCube.Succeeded())
		{
			VisualMesh->SetStaticMesh(fallbackCube.Object);
		}
	}

	HarvestBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("HarvestBar"));
	HarvestBar->SetupAttachment(VisualMesh);
	HarvestBar->SetWidgetSpace(EWidgetSpace::Screen);
	HarvestBar->SetDrawSize(FVector2D(160.f, 36.f));
	HarvestBar->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	HarvestBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HarvestBar->SetHiddenInGame(true);
}

void ACaveboundRockDeposit::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		HarvestWidget = CreateWidget<UCaveboundHarvestBar>(World, UCaveboundHarvestBar::StaticClass());
		if (HarvestBar && HarvestWidget)
		{
			HarvestBar->SetWidget(HarvestWidget);
		}
	}

	RollYield();
}

void ACaveboundRockDeposit::RollYield()
{
	// Small piles are quick. Large piles take longer to mine.
	const int32 choices[] = {12, 20, 32, 48};
	RockTotal = choices[FMath::RandRange(0, 3)];
	RockRemaining = RockTotal;
	TickTime = 0.f;
	bHarvesting = false;

	const float size = 0.9f + (RockTotal / 48.f) * 1.4f;
	const float verticalScale = size * 0.7f;
	float meshBottom = -50.f;
	float meshTop = 50.f;
	if (VisualMesh && VisualMesh->GetStaticMesh())
	{
		const FBoxSphereBounds meshBounds = VisualMesh->GetStaticMesh()->GetBounds();
		meshBottom = meshBounds.Origin.Z - meshBounds.BoxExtent.Z;
		meshTop = meshBounds.Origin.Z + meshBounds.BoxExtent.Z;
	}
	const float meshOffset = -meshBottom * verticalScale;
	const float barOffset = meshTop * verticalScale + 70.f;
	if (VisualMesh)
	{
		VisualMesh->SetRelativeScale3D(FVector(size, size, verticalScale));
		VisualMesh->SetRelativeLocation(FVector(0.f, 0.f, meshOffset));
	}

	if (HarvestBar)
	{
		HarvestBar->SetRelativeLocation(FVector(0.f, 0.f, barOffset));
	}

	HideBar();
}

bool ACaveboundRockDeposit::AddHarvestTime(float DeltaTime)
{
	if (bDepleted || DeltaTime <= 0.f)
	{
		return false;
	}

	bHarvesting = true;
	TickTime += DeltaTime;
	UpdateBar();

	if (TickTime < SecondsPerRock)
	{
		return false;
	}

	TickTime = 0.f;
	const int32 RocksHarvested = FMath::Min(RocksPerHarvest, RockRemaining);
	RockRemaining = FMath::Max(0, RockRemaining - RocksHarvested);

	if (UWorld* World = GetWorld())
	{
		if (ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>())
		{
			GameMode->GiveRock(RocksHarvested);
		}
	}

	if (RockRemaining > 0)
	{
		UpdateBar();
		return false;
	}

	FinishHarvest();
	return true;
}

void ACaveboundRockDeposit::StopHarvest()
{
	if (bDepleted)
	{
		return;
	}

	TickTime = 0.f;
	bHarvesting = false;
	HideBar();
}

void ACaveboundRockDeposit::FinishHarvest()
{
	bDepleted = true;
	bHarvesting = false;
	TickTime = 0.f;
	WavesUntilReturn = FMath::Clamp(2 + RockTotal / 16, 2, 4);

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	HideBar();
}

void ACaveboundRockDeposit::OnWaveEnded()
{
	if (!bDepleted)
	{
		return;
	}

	WavesUntilReturn -= 1;
	if (WavesUntilReturn <= 0)
	{
		WakeUp();
	}
}

void ACaveboundRockDeposit::WakeUp()
{
	bDepleted = false;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	RollYield();
}

void ACaveboundRockDeposit::UpdateBar()
{
	if (!HarvestBar || !HarvestWidget)
	{
		return;
	}

	HarvestBar->SetHiddenInGame(false);
	const float finished = static_cast<float>(RockTotal - RockRemaining);
	const float partial = SecondsPerRock > 0.f
		? (TickTime / SecondsPerRock) * FMath::Min(RocksPerHarvest, RockRemaining)
		: 0.f;
	const float progress = RockTotal > 0 ? (finished + partial) / static_cast<float>(RockTotal) : 0.f;
	HarvestWidget->SetHarvest(RockRemaining, progress);
}

void ACaveboundRockDeposit::HideBar()
{
	if (HarvestBar)
	{
		HarvestBar->SetHiddenInGame(true);
	}
}
