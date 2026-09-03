#pragma once

#include "CoreMinimal.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UObject;
struct FTimerHandle;

/**
 * Applies a red overlay material on hit, then removes it after Duration seconds
 */
class FCaveboundDamageFlash
{
public:
	static void FlashMeshes(UObject* WorldContext,const TArray<UStaticMeshComponent*>& Meshes,FTimerHandle& TimerHandle,FLinearColor FlashColor,
		float Duration);

	static void ClearFlash(const TArray<UStaticMeshComponent*>& Meshes);
};
