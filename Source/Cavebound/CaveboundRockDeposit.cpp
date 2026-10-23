#include "CaveboundRockDeposit.h"

#include "CaveboundGameMode.h"
#include "CaveboundHarvestBar.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> cubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (cubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(cubeMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> basicMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (basicMaterial.Succeeded())
	{
		if (UMaterialInstanceDynamic* rockColor = UMaterialInstanceDynamic::Create(basicMaterial.Object, this))
		{
			rockColor->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.45f, 0.42f, 0.38f, 1.f));
			VisualMesh->SetMaterial(0, rockColor);
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
	if (VisualMesh)
	{
		VisualMesh->SetRelativeScale3D(FVector(size, size, size * 0.7f));
		VisualMesh->SetRelativeLocation(FVector(0.f, 0.f, 50.f * size * 0.7f));
	}

	if (HarvestBar)
	{
		HarvestBar->SetRelativeLocation(FVector(0.f, 0.f, 50.f * size * 0.7f + 70.f));
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
	RockRemaining = FMath::Max(0, RockRemaining - 1);

	if (UWorld* World = GetWorld())
	{
		if (ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>())
		{
			GameMode->GiveRock(1);
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
	const float partial = SecondsPerRock > 0.f ? TickTime / SecondsPerRock : 0.f;
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
