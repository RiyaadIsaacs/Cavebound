#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;
class UObject;

/**
 * Custom shaders:
 * M_PoisonPulse: translucent unlit, lerps two greens over Time
 * M_TowerDamageDisplace: a wobble effect scaled by DamageAmount (0 healthy .. 1 damaged)
 * M_HoverOutline: translucent unlit fresnel rim for hover outlines
 */
namespace CaveboundShaderMaterials
{
	UMaterialInterface* GetPoisonPulseMaterial();
	UMaterialInterface* GetTowerDamageDisplaceMaterial();
	UMaterialInterface* GetHoverOutlineMaterial();

	// Create a MID parented to the poison pulse material. 
	class UMaterialInstanceDynamic* CreatePoisonPulseMID(UObject* Outer);

	// Create a MID parented to the tower damage material.
	class UMaterialInstanceDynamic* CreateTowerDamageDisplaceMID(UObject* Outer);

	// Create a MID parented to the hover outline material.
	class UMaterialInstanceDynamic* CreateHoverOutlineMID(UObject* Outer);
}
