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
 * M_RockMineDisplace: sink + crumble WPO scaled by MineAmount (0 full .. 1 empty)
 * M_MapEdgeFog: translucent unlit fog that is solid outside the map and fades inward
 */
namespace CaveboundShaderMaterials
{
	UMaterialInterface* GetPoisonPulseMaterial();
	UMaterialInterface* GetTowerDamageDisplaceMaterial();
	UMaterialInterface* GetTierAuraMaterial();
	UMaterialInterface* GetEnragePulseMaterial();
	UMaterialInterface* GetHoverOutlineMaterial();
	UMaterialInterface* GetRockMineDisplaceMaterial();
	UMaterialInterface* GetMapEdgeFogMaterial();

	class UMaterialInstanceDynamic* CreatePoisonPulseMID(UObject* Outer);
	class UMaterialInstanceDynamic* CreateTowerDamageDisplaceMID(UObject* Outer);
	class UMaterialInstanceDynamic* CreateTierAuraMID(UObject* Outer);
	class UMaterialInstanceDynamic* CreateEnragePulseMID(UObject* Outer);
	class UMaterialInstanceDynamic* CreateHoverOutlineMID(UObject* Outer);
	class UMaterialInstanceDynamic* CreateRockMineDisplaceMID(UObject* Outer);
	class UMaterialInstanceDynamic* CreateMapEdgeFogMID(UObject* Outer);
}
