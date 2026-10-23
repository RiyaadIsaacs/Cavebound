#include "CaveboundRockField.h"

#include "CaveboundRockDeposit.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"

UCaveboundRockField::UCaveboundRockField()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCaveboundRockField::BeginPlay()
{
	Super::BeginPlay();

	// The terrain blueprint fills CellIsPath during its own BeginPlay. Wait one tick so that grid exists.
	if (UWorld* world = GetWorld())
	{
		world->GetTimerManager().SetTimerForNextTick(this, &UCaveboundRockField::SpawnRocks);
	}
}

void UCaveboundRockField::SpawnRocks()
{
	if (bSpawned)
	{
		return;
	}

	AActor* terrain = GetOwner();
	UWorld* world = GetWorld();
	if (!terrain || !world)
	{
		return;
	}

	UClass* terrainClass = terrain->GetClass();
	FIntProperty* gridXProp = FindFProperty<FIntProperty>(terrainClass, TEXT("GridSizeX"));
	FIntProperty* gridYProp = FindFProperty<FIntProperty>(terrainClass, TEXT("GridSizeY"));
	FNumericProperty* cellProp = FindFProperty<FNumericProperty>(terrainClass, TEXT("CellSize"));
	FNumericProperty* halfWidthProp = FindFProperty<FNumericProperty>(terrainClass, TEXT("HalfWidth"));
	FNumericProperty* halfHeightProp = FindFProperty<FNumericProperty>(terrainClass, TEXT("HalfHeight"));
	FArrayProperty* pathProp = FindFProperty<FArrayProperty>(terrainClass, TEXT("CellIsPath"));
	if (!gridXProp || !gridYProp || !cellProp || !pathProp)
	{
		return;
	}

	const int32 gridX = gridXProp->GetPropertyValue_InContainer(terrain);
	const int32 gridY = gridYProp->GetPropertyValue_InContainer(terrain);
	const double cellSize = cellProp->GetFloatingPointPropertyValue(cellProp->ContainerPtrToValuePtr<void>(terrain));
	if (gridX <= 0 || gridY <= 0 || cellSize <= 0.0)
	{
		return;
	}

	double halfWidth = halfWidthProp ? halfWidthProp->GetFloatingPointPropertyValue(halfWidthProp->ContainerPtrToValuePtr<void>(terrain)) : 0.0;
	double halfHeight = halfHeightProp ? halfHeightProp->GetFloatingPointPropertyValue(halfHeightProp->ContainerPtrToValuePtr<void>(terrain)) : 0.0;
	if (halfWidth <= 0.0)
	{
		halfWidth = gridX * cellSize * 0.5;
	}
	if (halfHeight <= 0.0)
	{
		halfHeight = gridY * cellSize * 0.5;
	}

	FScriptArrayHelper pathCells(pathProp, pathProp->ContainerPtrToValuePtr<void>(terrain));
	FBoolProperty* pathFlag = CastField<FBoolProperty>(pathProp->Inner);
	if (!pathFlag || pathCells.Num() == 0)
	{
		if (SpawnTries < 8)
		{
			++SpawnTries;
			if (UWorld* retryWorld = GetWorld())
			{
				retryWorld->GetTimerManager().SetTimerForNextTick(this, &UCaveboundRockField::SpawnRocks);
			}
		}
		return;
	}

	// The terrain loop is inclusive, so a 30 grid has 31 points. Fall back if the array is the other size.
	int32 countX = gridX + 1;
	int32 countY = gridY + 1;
	if (pathCells.Num() == gridX * gridY)
	{
		countX = gridX;
		countY = gridY;
	}

	const FTransform terrainTransform = terrain->GetActorTransform();
	const FVector treeLocation = terrain->GetActorLocation();
	TArray<FVector> chosen;
	const int32 pathMargins[] = {2, 0};

	for (int32 pathMargin : pathMargins)
	{
		if (chosen.Num() > 0)
		{
			break;
		}

		for (int32 x = 2; x < countX - 2; x += 5)
		{
			for (int32 y = 2; y < countY - 2; y += 5)
			{
				if (chosen.Num() >= MaxRocks)
				{
					break;
				}

				const int32 index = x * countY + y;
				if (!pathCells.IsValidIndex(index) || pathFlag->GetPropertyValue(pathCells.GetRawPtr(index)))
				{
					continue;
				}

				bool bNearPath = false;
				for (int32 offsetX = -pathMargin; offsetX <= pathMargin && !bNearPath; ++offsetX)
				{
					for (int32 offsetY = -pathMargin; offsetY <= pathMargin; ++offsetY)
					{
						const int32 neighbor = (x + offsetX) * countY + (y + offsetY);
						if (pathCells.IsValidIndex(neighbor) && pathFlag->GetPropertyValue(pathCells.GetRawPtr(neighbor)))
						{
							bNearPath = true;
							break;
						}
					}
				}
				if (bNearPath)
				{
					continue;
				}

				const FVector localPoint(
					x * cellSize - halfWidth,
					y * cellSize - halfHeight,
					0.0);
				const FVector worldPoint = terrainTransform.TransformPosition(localPoint);
				if (FVector::Dist2D(worldPoint, treeLocation) < 700.f)
				{
					continue;
				}

				bool bTooClose = false;
				for (const FVector& other : chosen)
				{
					if (FVector::Dist2D(worldPoint, other) < MinSpacing)
					{
						bTooClose = true;
						break;
					}
				}
				if (bTooClose)
				{
					continue;
				}

				FActorSpawnParameters spawnParams;
				spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				spawnParams.Owner = terrain;
				if (world->SpawnActor<ACaveboundRockDeposit>(
					ACaveboundRockDeposit::StaticClass(),
					worldPoint,
					FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f),
					spawnParams))
				{
					chosen.Add(worldPoint);
				}
			}
		}
	}

	bSpawned = chosen.Num() > 0;
}
