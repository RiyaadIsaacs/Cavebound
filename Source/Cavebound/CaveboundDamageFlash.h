#pragma once

#include "CoreMinimal.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UObject;
struct FTimerHandle;

// flash state for a single mesh and material
struct FCaveboundMeshFlashState
{
	TWeakObjectPtr<UStaticMeshComponent> Mesh;
	int32 MaterialIndex = 0;
	FLinearColor OriginalColor = FLinearColor::White;
	TWeakObjectPtr<UMaterialInstanceDynamic> MID;
};

class FCaveboundDamageFlash
{
public:
	static void FlashMeshes(UObject* WorldContext,const TArray<UStaticMeshComponent*>& Meshes,FTimerHandle& TimerHandle,
		TArray<FCaveboundMeshFlashState>& Cache,
		FLinearColor FlashColor,
		float Duration);

	static void RestoreFlash(TArray<FCaveboundMeshFlashState>& Cache);
};
