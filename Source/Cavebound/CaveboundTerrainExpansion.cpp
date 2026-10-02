#include "CaveboundTerrainExpansion.h"
#include "Components/SplineComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogCaveboundSlots, Log, All);

namespace CaveboundTerrainExpansionPrivate
{
	static AActor* FindProceduralTerrain(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		UClass* TerrainClass = LoadClass<AActor>(
			nullptr,
			TEXT("/Game/Blueprints/BP_ProceduralTerrain.BP_ProceduralTerrain_C"));
		if (!TerrainClass)
		{
			return nullptr;
		}

		TArray<AActor*> Terrains;
		UGameplayStatics::GetAllActorsOfClass(World, TerrainClass, Terrains);
		return Terrains.Num() > 0 ? Terrains[0] : nullptr;
	}

	static bool ReadIntProperty(AActor* Actor, const FName Name, int32& OutValue)
	{
		const FIntProperty* Property = FindFProperty<FIntProperty>(Actor->GetClass(), Name);
		if (!Property)
		{
			return false;
		}
		OutValue = Property->GetPropertyValue_InContainer(Actor);
		return true;
	}

	static bool WriteIntProperty(AActor* Actor, const FName Name, int32 Value)
	{
		FIntProperty* Property = FindFProperty<FIntProperty>(Actor->GetClass(), Name);
		if (!Property)
		{
			return false;
		}
		Property->SetPropertyValue_InContainer(Actor, Value);
		return true;
	}

	static bool ReadFloatProperty(AActor* Actor, const FName Name, float& OutValue)
	{
		const FFloatProperty* Property = FindFProperty<FFloatProperty>(Actor->GetClass(), Name);
		if (!Property)
		{
			return false;
		}
		OutValue = Property->GetPropertyValue_InContainer(Actor);
		return true;
	}

	static bool WriteFloatProperty(AActor* Actor, const FName Name, float Value)
	{
		FFloatProperty* Property = FindFProperty<FFloatProperty>(Actor->GetClass(), Name);
		if (!Property)
		{
			return false;
		}
		Property->SetPropertyValue_InContainer(Actor, Value);
		return true;
	}

	static void CollectPathSplines(AActor* Terrain, TArray<USplineComponent*>& OutSplines)
	{
		OutSplines.Reset();
		if (!Terrain)
		{
			return;
		}

		TArray<USplineComponent*> Splines;
		Terrain->GetComponents<USplineComponent>(Splines);
		for (USplineComponent* Spline : Splines)
		{
			if (Spline && Spline->GetNumberOfSplinePoints() >= 2 && Spline->GetName().Contains(TEXT("Path")))
			{
				OutSplines.Add(Spline);
			}
		}
	}

	static void GatherExistingSlotLocations(UWorld* World, TArray<FVector>& OutLocations)
	{
		OutLocations.Reset();
		UClass* SlotClass = LoadClass<AActor>(nullptr, TEXT("/Game/Blueprints/BP_BuildSlot.BP_BuildSlot_C"));
		if (!World || !SlotClass)
		{
			return;
		}

		TArray<AActor*> Slots;
		UGameplayStatics::GetAllActorsOfClass(World, SlotClass, Slots);
		for (AActor* Slot : Slots)
		{
			if (Slot)
			{
				OutLocations.Add(Slot->GetActorLocation());
			}
		}
	}

	static bool IsFarEnough(const FVector& Candidate, const TArray<FVector>& Existing, float MinDistance)
	{
		const float MinDistSq = FMath::Square(MinDistance);
		for (const FVector& Loc : Existing)
		{
			if (FVector::DistSquared(Candidate, Loc) < MinDistSq)
			{
				return false;
			}
		}
		return true;
	}

	static void AssignTerrainReference(AActor* Slot, AActor* Terrain)
	{
		if (!Slot || !Terrain)
		{
			return;
		}

		if (FObjectProperty* TerrainProp = FindFProperty<FObjectProperty>(Slot->GetClass(), TEXT("Terrain")))
		{
			if (!TerrainProp->GetObjectPropertyValue_InContainer(Slot))
			{
				TerrainProp->SetObjectPropertyValue_InContainer(Slot, Terrain);
			}
		}
	}

	/**
	 * Grow the visible map: bump GridSize, update HalfWidth/HalfHeight,
	 * scale the Terrain mesh, and push Path1/2/3 endpoints outward.
	 * Avoids RerunConstructionScripts so existing pads/turrets stay intact.
	 */
	static bool ExpandMapVisually(AActor* Terrain, int32 ExtraCellsPerSide, int32 MaxGridSize, float& OutGrowDistance)
	{
		OutGrowDistance = 0.f;
		if (!Terrain || ExtraCellsPerSide <= 0)
		{
			return false;
		}

		int32 GridSizeX = 0;
		int32 GridSizeY = 0;
		if (!ReadIntProperty(Terrain, TEXT("GridSizeX"), GridSizeX)
			|| !ReadIntProperty(Terrain, TEXT("GridSizeY"), GridSizeY))
		{
			UE_LOG(LogCaveboundSlots, Warning, TEXT("ExpandMap: GridSizeX/Y missing on terrain"));
			return false;
		}

		const int32 OldX = GridSizeX;
		const int32 OldY = GridSizeY;
		const int32 NewX = FMath::Clamp(OldX + ExtraCellsPerSide, OldX, MaxGridSize);
		const int32 NewY = FMath::Clamp(OldY + ExtraCellsPerSide, OldY, MaxGridSize);
		if (NewX == OldX && NewY == OldY)
		{
			UE_LOG(LogCaveboundSlots, Log, TEXT("ExpandMap: already at MaxGridSize (%d)"), MaxGridSize);
			return false;
		}

		WriteIntProperty(Terrain, TEXT("GridSizeX"), NewX);
		WriteIntProperty(Terrain, TEXT("GridSizeY"), NewY);

		float CellSize = 100.f;
		ReadFloatProperty(Terrain, TEXT("CellSize"), CellSize);
		OutGrowDistance = ExtraCellsPerSide * CellSize;

		const float HalfWidth = NewX * CellSize * 0.5f;
		const float HalfHeight = NewY * CellSize * 0.5f;
		WriteFloatProperty(Terrain, TEXT("HalfWidth"), HalfWidth);
		WriteFloatProperty(Terrain, TEXT("HalfHeight"), HalfHeight);

		const float ScaleX = (OldX > 0) ? (static_cast<float>(NewX) / static_cast<float>(OldX)) : 1.f;
		const float ScaleY = (OldY > 0) ? (static_cast<float>(NewY) / static_cast<float>(OldY)) : 1.f;

		TArray<USceneComponent*> SceneComps;
		Terrain->GetComponents<USceneComponent>(SceneComps);
		for (USceneComponent* Comp : SceneComps)
		{
			if (!Comp)
			{
				continue;
			}

			// BP component is named Terrain (ProceduralMeshComponent).
			if (Comp->GetName().Contains(TEXT("Terrain")) && !Comp->IsA<USplineComponent>())
			{
				const FVector Current = Comp->GetRelativeScale3D();
				Comp->SetRelativeScale3D(FVector(Current.X * ScaleX, Current.Y * ScaleY, Current.Z));
			}
		}

		const FVector Origin = Terrain->GetActorLocation();
		TArray<USplineComponent*> Paths;
		CollectPathSplines(Terrain, Paths);
		for (USplineComponent* Spline : Paths)
		{
			const int32 NumPoints = Spline->GetNumberOfSplinePoints();
			if (NumPoints < 2)
			{
				continue;
			}

			auto PushPointOutward = [&](int32 PointIndex)
			{
				FVector Loc = Spline->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);
				FVector Dir = Loc - Origin;
				Dir.Z = 0.f;
				if (Dir.IsNearlyZero())
				{
					Dir = Spline->GetTangentAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);
					Dir.Z = 0.f;
					if (PointIndex == 0)
					{
						Dir *= -1.f;
					}
				}
				if (!Dir.Normalize())
				{
					return;
				}

				Loc += Dir * OutGrowDistance;
				Spline->SetLocationAtSplinePoint(PointIndex, Loc, ESplineCoordinateSpace::World, false);
			};

			PushPointOutward(0);
			PushPointOutward(NumPoints - 1);
			Spline->UpdateSpline();
		}

		UE_LOG(
			LogCaveboundSlots,
			Log,
			TEXT("ExpandMap: grid %dx%d -> %dx%d (scale %.2fx%.2f, grow=%.0fcm)"),
			OldX,
			OldY,
			NewX,
			NewY,
			ScaleX,
			ScaleY,
			OutGrowDistance);

		return true;
	}

	static AActor* CallTrySpawnSlotAtWorldLocation(AActor* Terrain, const FVector& WorldLocation)
	{
		if (!IsValid(Terrain))
		{
			return nullptr;
		}

		static const FName CandidateNames[] = {
			FName(TEXT("TrySpawnSlotAtLocation")),
			FName(TEXT("TrySpawnSlotAtWorldLocation")),
			FName(TEXT("SpawnBuildSlotAtLocation")),
		};

		for (const FName FuncName : CandidateNames)
		{
			UFunction* Func = Terrain->FindFunction(FuncName);
			if (!Func || Func->ParmsSize <= 0)
			{
				continue;
			}

			uint8* Params = static_cast<uint8*>(FMemory_Alloca(Func->ParmsSize));
			FMemory::Memzero(Params, Func->ParmsSize);

			bool bWroteLocation = false;
			for (TFieldIterator<FProperty> It(Func); It; ++It)
			{
				FProperty* Property = *It;
				if (!Property->HasAnyPropertyFlags(CPF_Parm) || Property->HasAnyPropertyFlags(CPF_ReturnParm))
				{
					continue;
				}

				if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
				{
					if (StructProp->Struct == TBaseStructure<FVector>::Get())
					{
						*StructProp->ContainerPtrToValuePtr<FVector>(Params) = WorldLocation;
						bWroteLocation = true;
					}
				}
			}

			if (!bWroteLocation)
			{
				continue;
			}

			Terrain->ProcessEvent(Func, Params);

			for (TFieldIterator<FProperty> It(Func); It; ++It)
			{
				FProperty* Property = *It;
				if (!Property->HasAnyPropertyFlags(CPF_Parm))
				{
					continue;
				}

				if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
				{
					if (AActor* Spawned = Cast<AActor>(ObjProp->GetObjectPropertyValue_InContainer(Params)))
					{
						return Spawned;
					}
				}
			}
		}

		return nullptr;
	}

	/** Prefer new pads near the outer ends of each path (the freshly expanded edge). */
	static int32 SpawnSlotsAlongPaths(
		UWorld* World,
		AActor* Terrain,
		float Spacing,
		float PathWidth,
		float SidePadding,
		int32 MaxNewSlots)
	{
		UClass* SlotClass = LoadClass<AActor>(nullptr, TEXT("/Game/Blueprints/BP_BuildSlot.BP_BuildSlot_C"));
		if (FClassProperty* ClassProp = FindFProperty<FClassProperty>(Terrain->GetClass(), TEXT("BuildSlotClass")))
		{
			if (UClass* FromTerrain = Cast<UClass>(ClassProp->GetPropertyValue_InContainer(Terrain)))
			{
				SlotClass = FromTerrain;
			}
		}

		if (!World || !SlotClass || MaxNewSlots <= 0)
		{
			return 0;
		}

		TArray<USplineComponent*> Paths;
		CollectPathSplines(Terrain, Paths);
		if (Paths.Num() == 0)
		{
			UE_LOG(LogCaveboundSlots, Warning, TEXT("ExpandBuildSlots: no Path splines on terrain"));
			return 0;
		}

		TArray<FVector> Existing;
		GatherExistingSlotLocations(World, Existing);
		const int32 ExistingCount = Existing.Num();

		const float Lateral = FMath::Max(50.f, PathWidth * 0.5f + SidePadding);
		const float Step = FMath::Max(150.f, Spacing);
		int32 Spawned = 0;

		// Walk from the far end of each path first so new pads sit on the expanded rim.
		for (USplineComponent* Spline : Paths)
		{
			if (Spawned >= MaxNewSlots)
			{
				break;
			}

			const float Length = Spline->GetSplineLength();
			for (float Dist = Length - Step * 0.5f; Dist > Step * 0.25f && Spawned < MaxNewSlots; Dist -= Step)
			{
				const FVector Centre = Spline->GetLocationAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World);
				FVector Tangent = Spline->GetTangentAtDistanceAlongSpline(Dist, ESplineCoordinateSpace::World);
				Tangent.Z = 0.f;
				if (Tangent.IsNearlyZero())
				{
					continue;
				}

				FVector Right = FVector::CrossProduct(FVector::UpVector, Tangent.GetSafeNormal());
				Right.Normalize();

				for (int32 Side = -1; Side <= 1; Side += 2)
				{
					if (Spawned >= MaxNewSlots)
					{
						break;
					}

					FVector Candidate = Centre + Right * (Lateral * static_cast<float>(Side));
					Candidate.Z = Centre.Z;

					if (!IsFarEnough(Candidate, Existing, Step * 0.9f))
					{
						continue;
					}

					AActor* NewSlot = CallTrySpawnSlotAtWorldLocation(Terrain, Candidate);
					if (!NewSlot)
					{
						FActorSpawnParameters SpawnParams;
						SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
						SpawnParams.Owner = Terrain;

						NewSlot = World->SpawnActor<AActor>(
							SlotClass,
							Candidate,
							Tangent.Rotation(),
							SpawnParams);
					}

					if (!NewSlot)
					{
						continue;
					}

					AssignTerrainReference(NewSlot, Terrain);
					Existing.Add(NewSlot->GetActorLocation());
					++Spawned;

					UE_LOG(LogCaveboundSlots, Log, TEXT("ExpandBuildSlots: spawned pad at %s"), *Candidate.ToCompactString());
				}
			}
		}

		UE_LOG(
			LogCaveboundSlots,
			Log,
			TEXT("ExpandBuildSlots: had %d pads, added %d / max %d"),
			ExistingCount,
			Spawned,
			MaxNewSlots);

		return Spawned;
	}
}

int32 FCaveboundTerrainExpansion::ExpandBuildSlots(
	UWorld* World,
	int32 ExtraCellsPerSide,
	float MinSlotSpacingMultiplier,
	float MinSlotSpacingFloor,
	int32 MaxGridSize,
	float PathSidePadding,
	int32 MaxNewSlots)
{
	using namespace CaveboundTerrainExpansionPrivate;

	AActor* Terrain = FindProceduralTerrain(World);
	if (!Terrain)
	{
		UE_LOG(LogCaveboundSlots, Warning, TEXT("ExpandBuildSlots: BP_ProceduralTerrain not found"));
		return 0;
	}

	float MinSlotSpacing = 400.f;
	ReadFloatProperty(Terrain, TEXT("MinSlotSpacing"), MinSlotSpacing);
	const float Tightened = FMath::Max(MinSlotSpacingFloor, MinSlotSpacing * MinSlotSpacingMultiplier);
	WriteFloatProperty(Terrain, TEXT("MinSlotSpacing"), Tightened);

	float PathWidth = 200.f;
	ReadFloatProperty(Terrain, TEXT("PathWidth"), PathWidth);

	float GrowDistance = 0.f;
	ExpandMapVisually(Terrain, ExtraCellsPerSide, MaxGridSize, GrowDistance);

	return SpawnSlotsAlongPaths(
		World,
		Terrain,
		Tightened,
		PathWidth,
		PathSidePadding,
		FMath::Max(0, MaxNewSlots));
}
