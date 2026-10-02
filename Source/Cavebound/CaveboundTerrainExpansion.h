#pragma once

#include "CoreMinimal.h"

/**
 * Grows BP_ProceduralTerrain visually (grid + mesh + path length)
 * and adds a small number of new build pads beside the lanes.
 */
class CAVEBOUND_API FCaveboundTerrainExpansion
{
public:

	static int32 ExpandBuildSlots(
		UWorld* World,
		int32 ExtraCellsPerSide = 2,
		float MinSlotSpacingMultiplier = 0.85f,
		float MinSlotSpacingFloor = 200.f,
		int32 MaxGridSize = 48,
		float PathSidePadding = 80.f,
		int32 MaxNewSlots = 2);
};
