#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;
class UObject;

/**
 * Custom shaders:
 * M_PoisonPulse: translucent unlit, lerps two greens over Time
 * M_TowerDamageDisplace: a wobble effect scaled by DamageAmount (0 healthy .. 1 damaged)
 * M_TierAura: translucent unlit pulse for boss tier aura (AuraColorA/B)
 * M_EnragePulse: purple emissive pulse + light WPO for boss enrage
 * M_HoverOutline: translucent unlit fresnel rim for hover outlines
 */
namespace CaveboundShaderMaterials
{
	UMaterialInterface* GetPoisonPulseMaterial();
	UMaterialInterface* GetTowerDamageDisplaceMaterial();
	UMaterialInterface* GetTierAuraMaterial();
	UMaterialInterface* GetEnragePulseMaterial();
	UMaterialInterface* GetHoverOutlineMaterial();

	class UMaterialInstanceDynamic* CreatePoisonPulseMID(UObject* Outer);
	class UMaterialInstanceDynamic* CreateTowerDamageDisplaceMID(UObject* Outer);
	class UMaterialInstanceDynamic* CreateTierAuraMID(UObject* Outer);
	class UMaterialInstanceDynamic* CreateEnragePulseMID(UObject* Outer);
	class UMaterialInstanceDynamic* CreateHoverOutlineMID(UObject* Outer);
}
