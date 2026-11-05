#pragma once

#include "CoreMinimal.h"

class AActor;
class UMeshComponent;
class UMaterialInterface;
class UWorld;

/**
 * Applies /Game/Assets/Materials/PostFX/M_Outline as a persistent mesh overlay.
 * Temporary overlays (hover, damage flash, etc.) should call Restore() when cleared.
 */
namespace CaveboundModelOutline
{
	UMaterialInterface* GetMaterial();

	void ApplyToMesh(UMeshComponent* Mesh);
	void ApplyToActor(AActor* Actor);
	void RestoreMesh(UMeshComponent* Mesh);
	void RestoreActor(AActor* Actor);

	/** Apply to common gameplay classes currently in the world (including BP build slots). */
	void EnsureWorld(UWorld* World);
}
