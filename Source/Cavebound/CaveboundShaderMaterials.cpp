#include "CaveboundShaderMaterials.h"

#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Paths.h"
#include "Templates/Function.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#endif

#if WITH_EDITORONLY_DATA
#include "Engine/Texture2D.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSine.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionCustom.h"
#endif

namespace CaveboundShaderMaterials
{
	static constexpr const TCHAR* PoisonPulsePackage = TEXT("/Game/Assets/Materials/M_PoisonPulse");
	static constexpr const TCHAR* PoisonPulsePath = TEXT("/Game/Assets/Materials/M_PoisonPulse.M_PoisonPulse");
	static constexpr const TCHAR* TowerDamagePackage = TEXT("/Game/Assets/Materials/M_TowerDamageDisplace");
	static constexpr const TCHAR* TowerDamagePath = TEXT("/Game/Assets/Materials/M_TowerDamageDisplace.M_TowerDamageDisplace");
	// V2: travelling radial ring waves (V1 was colour-pulse only / stale cache risk).
	static constexpr const TCHAR* TierAuraPackage = TEXT("/Game/Assets/Materials/M_TierAuraV2");
	static constexpr const TCHAR* TierAuraPath = TEXT("/Game/Assets/Materials/M_TierAuraV2.M_TierAuraV2");
	static constexpr const TCHAR* EnragePulsePackage = TEXT("/Game/Assets/Materials/M_EnragePulse");
	static constexpr const TCHAR* EnragePulsePath = TEXT("/Game/Assets/Materials/M_EnragePulse.M_EnragePulse");
	// V3: Intensity drives emissive only; OpacityBoost drives rim strength (V2 was too faint).
	static constexpr const TCHAR* HoverOutlinePackage = TEXT("/Game/Assets/Materials/M_HoverOutlineV3");
	static constexpr const TCHAR* HoverOutlinePath = TEXT("/Game/Assets/Materials/M_HoverOutlineV3.M_HoverOutlineV3");
	// V2: sink + colour only (V1 cached a time-based crumble that kept oscillating at rest).
	static constexpr const TCHAR* RockMinePackage = TEXT("/Game/Assets/Materials/M_RockMineDisplaceV2");
	static constexpr const TCHAR* RockMinePath = TEXT("/Game/Assets/Materials/M_RockMineDisplaceV2.M_RockMineDisplaceV2");
	static constexpr const TCHAR* TowerTexturePath = TEXT("/Game/Assets/Models/Towers/T_FortressElves.T_FortressElves");
	static constexpr const TCHAR* RockTexturePath = TEXT("/Game/Assets/Models/Stones/Texture__1_.Texture__1_");

#if WITH_EDITORONLY_DATA
	static void FinalizeMaterial(UMaterial* Material)
	{
		if (!Material)
		{
			return;
		}

		bool bNeedsRecompile = false;
		Material->SetMaterialUsage(bNeedsRecompile, MATUSAGE_StaticMesh);
		Material->PreEditChange(nullptr);
		Material->PostEditChange();
	}

#if WITH_EDITOR
	static void TrySaveMaterialAsset(UMaterial* Material, const TCHAR* LongPackageName)
	{
		if (!Material)
		{
			return;
		}

		UPackage* Package = Material->GetOutermost();
		if (!Package)
		{
			return;
		}

		Package->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Material);

		const FString Filename = FPackageName::LongPackageNameToFilename(
			LongPackageName,
			FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		UPackage::SavePackage(Package, Material, *Filename, SaveArgs);
	}
#endif

	static UMaterial* BuildPoisonPulseMaterial(UObject* Outer, FName AssetName)
	{
		UMaterial* Material = NewObject<UMaterial>(Outer, AssetName, RF_Public | RF_Standalone);
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;
		Material->TranslucencyLightingMode = TLM_Surface;
		Material->bAutomaticallySetUsageInEditor = true;

		UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
		if (!EditorData)
		{
			return Material;
		}

		UMaterialExpressionVectorParameter* GreenA = NewObject<UMaterialExpressionVectorParameter>(Material);
		GreenA->ParameterName = TEXT("GreenA");
		GreenA->DefaultValue = FLinearColor(0.05f, 0.55f, 0.12f, 1.f);
		EditorData->ExpressionCollection.AddExpression(GreenA);

		UMaterialExpressionVectorParameter* GreenB = NewObject<UMaterialExpressionVectorParameter>(Material);
		GreenB->ParameterName = TEXT("GreenB");
		GreenB->DefaultValue = FLinearColor(0.25f, 0.95f, 0.35f, 1.f);
		EditorData->ExpressionCollection.AddExpression(GreenB);

		UMaterialExpressionScalarParameter* PulseSpeed = NewObject<UMaterialExpressionScalarParameter>(Material);
		PulseSpeed->ParameterName = TEXT("PulseSpeed");
		PulseSpeed->DefaultValue = 1.5f;
		EditorData->ExpressionCollection.AddExpression(PulseSpeed);

		UMaterialExpressionScalarParameter* Opacity = NewObject<UMaterialExpressionScalarParameter>(Material);
		Opacity->ParameterName = TEXT("Opacity");
		Opacity->DefaultValue = 0.3f;
		EditorData->ExpressionCollection.AddExpression(Opacity);

		UMaterialExpressionTime* TimeNode = NewObject<UMaterialExpressionTime>(Material);
		EditorData->ExpressionCollection.AddExpression(TimeNode);

		UMaterialExpressionMultiply* TimeScaled = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(TimeScaled);
		TimeScaled->A.Connect(0, TimeNode);
		TimeScaled->B.Connect(0, PulseSpeed);

		UMaterialExpressionSine* SineNode = NewObject<UMaterialExpressionSine>(Material);
		EditorData->ExpressionCollection.AddExpression(SineNode);
		SineNode->Input.Connect(0, TimeScaled);

		UMaterialExpressionConstant* One = NewObject<UMaterialExpressionConstant>(Material);
		One->R = 1.f;
		EditorData->ExpressionCollection.AddExpression(One);

		UMaterialExpressionAdd* SinePlusOne = NewObject<UMaterialExpressionAdd>(Material);
		EditorData->ExpressionCollection.AddExpression(SinePlusOne);
		SinePlusOne->A.Connect(0, SineNode);
		SinePlusOne->B.Connect(0, One);

		UMaterialExpressionConstant* Half = NewObject<UMaterialExpressionConstant>(Material);
		Half->R = 0.5f;
		EditorData->ExpressionCollection.AddExpression(Half);

		UMaterialExpressionMultiply* Alpha = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(Alpha);
		Alpha->A.Connect(0, SinePlusOne);
		Alpha->B.Connect(0, Half);

		UMaterialExpressionLinearInterpolate* ColourLerp = NewObject<UMaterialExpressionLinearInterpolate>(Material);
		EditorData->ExpressionCollection.AddExpression(ColourLerp);
		ColourLerp->A.Connect(0, GreenA);
		ColourLerp->B.Connect(0, GreenB);
		ColourLerp->Alpha.Connect(0, Alpha);

		ColourLerp->ConnectExpression(&EditorData->EmissiveColor, 0);
		ColourLerp->ConnectExpression(&EditorData->BaseColor, 0);
		Opacity->ConnectExpression(&EditorData->Opacity, 0);

		FinalizeMaterial(Material);
		return Material;
	}

	static UMaterial* BuildTierAuraMaterial(UObject* Outer, FName AssetName)
	{
		UMaterial* Material = NewObject<UMaterial>(Outer, AssetName, RF_Public | RF_Standalone);
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;
		Material->TranslucencyLightingMode = TLM_Surface;
		Material->bAutomaticallySetUsageInEditor = true;

		UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
		if (!EditorData)
		{
			return Material;
		}

		UMaterialExpressionVectorParameter* AuraA = NewObject<UMaterialExpressionVectorParameter>(Material);
		AuraA->ParameterName = TEXT("AuraColorA");
		AuraA->DefaultValue = FLinearColor(1.f, 1.f, 1.f, 1.f);
		EditorData->ExpressionCollection.AddExpression(AuraA);

		UMaterialExpressionVectorParameter* AuraB = NewObject<UMaterialExpressionVectorParameter>(Material);
		AuraB->ParameterName = TEXT("AuraColorB");
		AuraB->DefaultValue = FLinearColor(0.85f, 0.9f, 1.f, 1.f);
		EditorData->ExpressionCollection.AddExpression(AuraB);

		UMaterialExpressionScalarParameter* PulseSpeed = NewObject<UMaterialExpressionScalarParameter>(Material);
		PulseSpeed->ParameterName = TEXT("PulseSpeed");
		PulseSpeed->DefaultValue = 1.2f;
		EditorData->ExpressionCollection.AddExpression(PulseSpeed);

		UMaterialExpressionScalarParameter* Opacity = NewObject<UMaterialExpressionScalarParameter>(Material);
		Opacity->ParameterName = TEXT("Opacity");
		Opacity->DefaultValue = 0.05f;
		EditorData->ExpressionCollection.AddExpression(Opacity);

		UMaterialExpressionScalarParameter* WaveFreq = NewObject<UMaterialExpressionScalarParameter>(Material);
		WaveFreq->ParameterName = TEXT("WaveFreq");
		WaveFreq->DefaultValue = 0.08f;
		EditorData->ExpressionCollection.AddExpression(WaveFreq);

		UMaterialExpressionScalarParameter* WaveSpeed = NewObject<UMaterialExpressionScalarParameter>(Material);
		WaveSpeed->ParameterName = TEXT("WaveSpeed");
		WaveSpeed->DefaultValue = 2.0f;
		EditorData->ExpressionCollection.AddExpression(WaveSpeed);

		UMaterialExpressionScalarParameter* WaveAmount = NewObject<UMaterialExpressionScalarParameter>(Material);
		WaveAmount->ParameterName = TEXT("WaveAmount");
		WaveAmount->DefaultValue = 12.f;
		EditorData->ExpressionCollection.AddExpression(WaveAmount);

		UMaterialExpressionTime* TimeNode = NewObject<UMaterialExpressionTime>(Material);
		EditorData->ExpressionCollection.AddExpression(TimeNode);

		UMaterialExpressionMultiply* TimeScaled = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(TimeScaled);
		TimeScaled->A.Connect(0, TimeNode);
		TimeScaled->B.Connect(0, PulseSpeed);

		UMaterialExpressionSine* SineNode = NewObject<UMaterialExpressionSine>(Material);
		EditorData->ExpressionCollection.AddExpression(SineNode);
		SineNode->Input.Connect(0, TimeScaled);

		UMaterialExpressionConstant* One = NewObject<UMaterialExpressionConstant>(Material);
		One->R = 1.f;
		EditorData->ExpressionCollection.AddExpression(One);

		UMaterialExpressionAdd* SinePlusOne = NewObject<UMaterialExpressionAdd>(Material);
		EditorData->ExpressionCollection.AddExpression(SinePlusOne);
		SinePlusOne->A.Connect(0, SineNode);
		SinePlusOne->B.Connect(0, One);

		UMaterialExpressionConstant* Half = NewObject<UMaterialExpressionConstant>(Material);
		Half->R = 0.5f;
		EditorData->ExpressionCollection.AddExpression(Half);

		UMaterialExpressionMultiply* Alpha = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(Alpha);
		Alpha->A.Connect(0, SinePlusOne);
		Alpha->B.Connect(0, Half);

		UMaterialExpressionLinearInterpolate* ColourLerp = NewObject<UMaterialExpressionLinearInterpolate>(Material);
		EditorData->ExpressionCollection.AddExpression(ColourLerp);
		ColourLerp->A.Connect(0, AuraA);
		ColourLerp->B.Connect(0, AuraB);
		ColourLerp->Alpha.Connect(0, Alpha);

		ColourLerp->ConnectExpression(&EditorData->EmissiveColor, 0);
		ColourLerp->ConnectExpression(&EditorData->BaseColor, 0);
		Opacity->ConnectExpression(&EditorData->Opacity, 0);

		// Traveling radial waves: Normal * sin(Z * WaveFreq + Time * WaveSpeed) * WaveAmount
		UMaterialExpressionWorldPosition* WorldPos = NewObject<UMaterialExpressionWorldPosition>(Material);
		EditorData->ExpressionCollection.AddExpression(WorldPos);

		UMaterialExpressionComponentMask* PosZ = NewObject<UMaterialExpressionComponentMask>(Material);
		PosZ->R = false;
		PosZ->G = false;
		PosZ->B = true;
		PosZ->A = false;
		EditorData->ExpressionCollection.AddExpression(PosZ);
		PosZ->Input.Connect(0, WorldPos);

		UMaterialExpressionMultiply* ZScaled = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(ZScaled);
		ZScaled->A.Connect(0, PosZ);
		ZScaled->B.Connect(0, WaveFreq);

		UMaterialExpressionMultiply* WaveTime = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(WaveTime);
		WaveTime->A.Connect(0, TimeNode);
		WaveTime->B.Connect(0, WaveSpeed);

		UMaterialExpressionAdd* WavePhase = NewObject<UMaterialExpressionAdd>(Material);
		EditorData->ExpressionCollection.AddExpression(WavePhase);
		WavePhase->A.Connect(0, ZScaled);
		WavePhase->B.Connect(0, WaveTime);

		UMaterialExpressionSine* WaveSine = NewObject<UMaterialExpressionSine>(Material);
		EditorData->ExpressionCollection.AddExpression(WaveSine);
		WaveSine->Input.Connect(0, WavePhase);

		UMaterialExpressionMultiply* WaveScaled = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(WaveScaled);
		WaveScaled->A.Connect(0, WaveSine);
		WaveScaled->B.Connect(0, WaveAmount);

		UMaterialExpressionVertexNormalWS* NormalWS = NewObject<UMaterialExpressionVertexNormalWS>(Material);
		EditorData->ExpressionCollection.AddExpression(NormalWS);

		UMaterialExpressionMultiply* WaveOffset = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(WaveOffset);
		WaveOffset->A.Connect(0, NormalWS);
		WaveOffset->B.Connect(0, WaveScaled);
		WaveOffset->ConnectExpression(&EditorData->WorldPositionOffset, 0);

		FinalizeMaterial(Material);
		return Material;
	}

	static UMaterial* BuildEnragePulseMaterial(UObject* Outer, FName AssetName)
	{
		UMaterial* Material = NewObject<UMaterial>(Outer, AssetName, RF_Public | RF_Standalone);
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Opaque;
		Material->SetShadingModel(MSM_DefaultLit);
		Material->TwoSided = false;
		Material->bAutomaticallySetUsageInEditor = true;

		UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
		if (!EditorData)
		{
			return Material;
		}

		UMaterialExpressionVectorParameter* PurpleA = NewObject<UMaterialExpressionVectorParameter>(Material);
		PurpleA->ParameterName = TEXT("PurpleA");
		PurpleA->DefaultValue = FLinearColor(0.35f, 0.05f, 0.55f, 1.f);
		EditorData->ExpressionCollection.AddExpression(PurpleA);

		UMaterialExpressionVectorParameter* PurpleB = NewObject<UMaterialExpressionVectorParameter>(Material);
		PurpleB->ParameterName = TEXT("PurpleB");
		PurpleB->DefaultValue = FLinearColor(0.85f, 0.2f, 1.f, 1.f);
		EditorData->ExpressionCollection.AddExpression(PurpleB);

		UMaterialExpressionVectorParameter* GlowColor = NewObject<UMaterialExpressionVectorParameter>(Material);
		GlowColor->ParameterName = TEXT("_EnrageGlowColor");
		GlowColor->DefaultValue = FLinearColor(0.65f, 0.1f, 1.f, 1.f);
		EditorData->ExpressionCollection.AddExpression(GlowColor);

		UMaterialExpressionScalarParameter* PulseSpeed = NewObject<UMaterialExpressionScalarParameter>(Material);
		PulseSpeed->ParameterName = TEXT("PulseSpeed");
		PulseSpeed->DefaultValue = 2.5f;
		EditorData->ExpressionCollection.AddExpression(PulseSpeed);

		UMaterialExpressionScalarParameter* Displace = NewObject<UMaterialExpressionScalarParameter>(Material);
		Displace->ParameterName = TEXT("DisplacementStrength");
		Displace->DefaultValue = 4.f;
		EditorData->ExpressionCollection.AddExpression(Displace);

		UMaterialExpressionTime* TimeNode = NewObject<UMaterialExpressionTime>(Material);
		EditorData->ExpressionCollection.AddExpression(TimeNode);

		UMaterialExpressionMultiply* TimeScaled = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(TimeScaled);
		TimeScaled->A.Connect(0, TimeNode);
		TimeScaled->B.Connect(0, PulseSpeed);

		UMaterialExpressionSine* SineNode = NewObject<UMaterialExpressionSine>(Material);
		EditorData->ExpressionCollection.AddExpression(SineNode);
		SineNode->Input.Connect(0, TimeScaled);

		UMaterialExpressionConstant* One = NewObject<UMaterialExpressionConstant>(Material);
		One->R = 1.f;
		EditorData->ExpressionCollection.AddExpression(One);

		UMaterialExpressionAdd* SinePlusOne = NewObject<UMaterialExpressionAdd>(Material);
		EditorData->ExpressionCollection.AddExpression(SinePlusOne);
		SinePlusOne->A.Connect(0, SineNode);
		SinePlusOne->B.Connect(0, One);

		UMaterialExpressionConstant* Half = NewObject<UMaterialExpressionConstant>(Material);
		Half->R = 0.5f;
		EditorData->ExpressionCollection.AddExpression(Half);

		UMaterialExpressionMultiply* Alpha = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(Alpha);
		Alpha->A.Connect(0, SinePlusOne);
		Alpha->B.Connect(0, Half);

		UMaterialExpressionLinearInterpolate* ColourLerp = NewObject<UMaterialExpressionLinearInterpolate>(Material);
		EditorData->ExpressionCollection.AddExpression(ColourLerp);
		ColourLerp->A.Connect(0, PurpleA);
		ColourLerp->B.Connect(0, PurpleB);
		ColourLerp->Alpha.Connect(0, Alpha);
		ColourLerp->ConnectExpression(&EditorData->BaseColor, 0);

		UMaterialExpressionMultiply* Emissive = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(Emissive);
		Emissive->A.Connect(0, GlowColor);
		Emissive->B.Connect(0, Alpha);
		Emissive->ConnectExpression(&EditorData->EmissiveColor, 0);

		UMaterialExpressionVertexNormalWS* NormalWS = NewObject<UMaterialExpressionVertexNormalWS>(Material);
		EditorData->ExpressionCollection.AddExpression(NormalWS);

		UMaterialExpressionMultiply* WpoAmount = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(WpoAmount);
		WpoAmount->A.Connect(0, SineNode);
		WpoAmount->B.Connect(0, Displace);

		UMaterialExpressionMultiply* Wpo = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(Wpo);
		Wpo->A.Connect(0, NormalWS);
		Wpo->B.Connect(0, WpoAmount);
		Wpo->ConnectExpression(&EditorData->WorldPositionOffset, 0);

		bool bNeedsRecompile = false;
		Material->SetMaterialUsage(bNeedsRecompile, MATUSAGE_SkeletalMesh);
		FinalizeMaterial(Material);
		return Material;
	}

	static UMaterial* BuildTowerDamageDisplaceMaterial(UObject* Outer, FName AssetName)
	{
		UMaterial* Material = NewObject<UMaterial>(Outer, AssetName, RF_Public | RF_Standalone);
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Opaque;
		Material->SetShadingModel(MSM_DefaultLit);
		Material->TwoSided = false;
		Material->bAutomaticallySetUsageInEditor = true;

		UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
		if (!EditorData)
		{
			return Material;
		}

		UMaterialExpressionScalarParameter* DamageAmount = NewObject<UMaterialExpressionScalarParameter>(Material);
		DamageAmount->ParameterName = TEXT("DamageAmount");
		DamageAmount->DefaultValue = 0.f;
		EditorData->ExpressionCollection.AddExpression(DamageAmount);

		UMaterialExpressionScalarParameter* MaxDisplace = NewObject<UMaterialExpressionScalarParameter>(Material);
		MaxDisplace->ParameterName = TEXT("MaxDisplace");
		MaxDisplace->DefaultValue = 10.f;
		EditorData->ExpressionCollection.AddExpression(MaxDisplace);

		UMaterialExpressionScalarParameter* NoiseScale = NewObject<UMaterialExpressionScalarParameter>(Material);
		NoiseScale->ParameterName = TEXT("NoiseScale");
		NoiseScale->DefaultValue = 0.04f;
		EditorData->ExpressionCollection.AddExpression(NoiseScale);

		UMaterialExpressionScalarParameter* WobbleSpeed = NewObject<UMaterialExpressionScalarParameter>(Material);
		WobbleSpeed->ParameterName = TEXT("WobbleSpeed");
		WobbleSpeed->DefaultValue = 3.f;
		EditorData->ExpressionCollection.AddExpression(WobbleSpeed);

		UMaterialExpressionWorldPosition* WorldPos = NewObject<UMaterialExpressionWorldPosition>(Material);
		EditorData->ExpressionCollection.AddExpression(WorldPos);

		UMaterialExpressionVertexNormalWS* NormalWS = NewObject<UMaterialExpressionVertexNormalWS>(Material);
		EditorData->ExpressionCollection.AddExpression(NormalWS);

		UMaterialExpressionTime* TimeNode = NewObject<UMaterialExpressionTime>(Material);
		EditorData->ExpressionCollection.AddExpression(TimeNode);

		UMaterialExpressionMultiply* PosScaled = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(PosScaled);
		PosScaled->A.Connect(0, WorldPos);
		PosScaled->B.Connect(0, NoiseScale);

		UMaterialExpressionMultiply* TimeScaled = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(TimeScaled);
		TimeScaled->A.Connect(0, TimeNode);
		TimeScaled->B.Connect(0, WobbleSpeed);

		UMaterialExpressionAdd* Phase = NewObject<UMaterialExpressionAdd>(Material);
		EditorData->ExpressionCollection.AddExpression(Phase);
		Phase->A.Connect(0, PosScaled);
		Phase->B.Connect(0, TimeScaled);

		UMaterialExpressionSine* Wave = NewObject<UMaterialExpressionSine>(Material);
		EditorData->ExpressionCollection.AddExpression(Wave);
		Wave->Input.Connect(0, Phase);

		UMaterialExpressionMultiply* WaveByDamage = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(WaveByDamage);
		WaveByDamage->A.Connect(0, Wave);
		WaveByDamage->B.Connect(0, DamageAmount);

		UMaterialExpressionMultiply* WaveByMax = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(WaveByMax);
		WaveByMax->A.Connect(0, WaveByDamage);
		WaveByMax->B.Connect(0, MaxDisplace);

		UMaterialExpressionMultiply* Offset = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(Offset);
		Offset->A.Connect(0, NormalWS);
		Offset->B.Connect(0, WaveByMax);
		Offset->ConnectExpression(&EditorData->WorldPositionOffset, 0);

		// Colour tint is driven separately so it can wait until ~50% HP.
		UMaterialExpressionScalarParameter* ColourDamageAmount = NewObject<UMaterialExpressionScalarParameter>(Material);
		ColourDamageAmount->ParameterName = TEXT("ColourDamageAmount");
		ColourDamageAmount->DefaultValue = 0.f;
		EditorData->ExpressionCollection.AddExpression(ColourDamageAmount);

		UMaterialExpressionVectorParameter* HealthyColour = NewObject<UMaterialExpressionVectorParameter>(Material);
		HealthyColour->ParameterName = TEXT("HealthyColour");
		HealthyColour->DefaultValue = FLinearColor(0.55f, 0.5f, 0.42f, 1.f);
		EditorData->ExpressionCollection.AddExpression(HealthyColour);

		UMaterialExpressionVectorParameter* DamagedColour = NewObject<UMaterialExpressionVectorParameter>(Material);
		DamagedColour->ParameterName = TEXT("DamagedColour");
		DamagedColour->DefaultValue = FLinearColor(0.35f, 0.12f, 0.08f, 1.f);
		EditorData->ExpressionCollection.AddExpression(DamagedColour);

		if (UTexture2D* TowerTex = LoadObject<UTexture2D>(nullptr, TowerTexturePath))
		{
			UMaterialExpressionTextureSample* TexSample = NewObject<UMaterialExpressionTextureSample>(Material);
			TexSample->Texture = TowerTex;
			EditorData->ExpressionCollection.AddExpression(TexSample);

			UMaterialExpressionLinearInterpolate* TexTint = NewObject<UMaterialExpressionLinearInterpolate>(Material);
			EditorData->ExpressionCollection.AddExpression(TexTint);
			TexTint->A.Connect(0, TexSample);
			TexTint->B.Connect(0, DamagedColour);
			TexTint->Alpha.Connect(0, ColourDamageAmount);
			TexTint->ConnectExpression(&EditorData->BaseColor, 0);
		}
		else
		{
			UMaterialExpressionLinearInterpolate* ColourLerp = NewObject<UMaterialExpressionLinearInterpolate>(Material);
			EditorData->ExpressionCollection.AddExpression(ColourLerp);
			ColourLerp->A.Connect(0, HealthyColour);
			ColourLerp->B.Connect(0, DamagedColour);
			ColourLerp->Alpha.Connect(0, ColourDamageAmount);
			ColourLerp->ConnectExpression(&EditorData->BaseColor, 0);
		}

		FinalizeMaterial(Material);
		return Material;
	}

	static UMaterial* BuildRockMineDisplaceMaterial(UObject* Outer, FName AssetName)
	{
		UMaterial* Material = NewObject<UMaterial>(Outer, AssetName, RF_Public | RF_Standalone);
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Opaque;
		Material->SetShadingModel(MSM_DefaultLit);
		Material->TwoSided = false;
		Material->bAutomaticallySetUsageInEditor = true;

		UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
		if (!EditorData)
		{
			return Material;
		}

		// 0 = untouched pile, 1 = fully mined. No time-based motion — rocks stay still.
		UMaterialExpressionScalarParameter* MineAmount = NewObject<UMaterialExpressionScalarParameter>(Material);
		MineAmount->ParameterName = TEXT("MineAmount");
		MineAmount->DefaultValue = 0.f;
		EditorData->ExpressionCollection.AddExpression(MineAmount);

		UMaterialExpressionScalarParameter* MaxSink = NewObject<UMaterialExpressionScalarParameter>(Material);
		MaxSink->ParameterName = TEXT("MaxSink");
		MaxSink->DefaultValue = 28.f;
		EditorData->ExpressionCollection.AddExpression(MaxSink);

		UMaterialExpressionConstant3Vector* Down = NewObject<UMaterialExpressionConstant3Vector>(Material);
		Down->Constant = FLinearColor(0.f, 0.f, -1.f, 0.f);
		EditorData->ExpressionCollection.AddExpression(Down);

		UMaterialExpressionMultiply* SinkAmount = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(SinkAmount);
		SinkAmount->A.Connect(0, MineAmount);
		SinkAmount->B.Connect(0, MaxSink);

		UMaterialExpressionMultiply* Sink = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(Sink);
		Sink->A.Connect(0, Down);
		Sink->B.Connect(0, SinkAmount);
		Sink->ConnectExpression(&EditorData->WorldPositionOffset, 0);

		UMaterialExpressionVectorParameter* HealthyColour = NewObject<UMaterialExpressionVectorParameter>(Material);
		HealthyColour->ParameterName = TEXT("HealthyColour");
		HealthyColour->DefaultValue = FLinearColor(0.55f, 0.52f, 0.48f, 1.f);
		EditorData->ExpressionCollection.AddExpression(HealthyColour);

		UMaterialExpressionVectorParameter* WornColour = NewObject<UMaterialExpressionVectorParameter>(Material);
		WornColour->ParameterName = TEXT("WornColour");
		WornColour->DefaultValue = FLinearColor(0.28f, 0.24f, 0.2f, 1.f);
		EditorData->ExpressionCollection.AddExpression(WornColour);

		if (UTexture2D* RockTex = LoadObject<UTexture2D>(nullptr, RockTexturePath))
		{
			UMaterialExpressionTextureSample* TexSample = NewObject<UMaterialExpressionTextureSample>(Material);
			TexSample->Texture = RockTex;
			EditorData->ExpressionCollection.AddExpression(TexSample);

			UMaterialExpressionLinearInterpolate* TexTint = NewObject<UMaterialExpressionLinearInterpolate>(Material);
			EditorData->ExpressionCollection.AddExpression(TexTint);
			TexTint->A.Connect(0, TexSample);
			TexTint->B.Connect(0, WornColour);
			TexTint->Alpha.Connect(0, MineAmount);
			TexTint->ConnectExpression(&EditorData->BaseColor, 0);
		}
		else
		{
			UMaterialExpressionLinearInterpolate* ColourLerp = NewObject<UMaterialExpressionLinearInterpolate>(Material);
			EditorData->ExpressionCollection.AddExpression(ColourLerp);
			ColourLerp->A.Connect(0, HealthyColour);
			ColourLerp->B.Connect(0, WornColour);
			ColourLerp->Alpha.Connect(0, MineAmount);
			ColourLerp->ConnectExpression(&EditorData->BaseColor, 0);
		}

		FinalizeMaterial(Material);
		return Material;
	}

	static UMaterial* BuildHoverOutlineMaterial(UObject* Outer, FName AssetName)
	{
		UMaterial* Material = NewObject<UMaterial>(Outer, AssetName, RF_Public | RF_Standalone);
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;
		Material->TranslucencyLightingMode = TLM_Surface;
		Material->bAutomaticallySetUsageInEditor = true;

		UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
		if (!EditorData)
		{
			return Material;
		}

		UMaterialExpressionVectorParameter* OutlineColor = NewObject<UMaterialExpressionVectorParameter>(Material);
		OutlineColor->ParameterName = TEXT("OutlineColor");
		OutlineColor->DefaultValue = FLinearColor(1.f, 1.f, 1.f, 1.f);
		EditorData->ExpressionCollection.AddExpression(OutlineColor);

		UMaterialExpressionScalarParameter* Intensity = NewObject<UMaterialExpressionScalarParameter>(Material);
		Intensity->ParameterName = TEXT("Intensity");
		Intensity->DefaultValue = 5.f;
		EditorData->ExpressionCollection.AddExpression(Intensity);

		UMaterialExpressionScalarParameter* OpacityBoost = NewObject<UMaterialExpressionScalarParameter>(Material);
		OpacityBoost->ParameterName = TEXT("OpacityBoost");
		OpacityBoost->DefaultValue = 2.75f;
		EditorData->ExpressionCollection.AddExpression(OpacityBoost);

		UMaterialExpressionScalarParameter* Exponent = NewObject<UMaterialExpressionScalarParameter>(Material);
		Exponent->ParameterName = TEXT("Exponent");
		Exponent->DefaultValue = 2.4f;
		EditorData->ExpressionCollection.AddExpression(Exponent);

		// Soft fill so the tint reads even on faces pointed at the camera.
		UMaterialExpressionScalarParameter* MinOpacity = NewObject<UMaterialExpressionScalarParameter>(Material);
		MinOpacity->ParameterName = TEXT("MinOpacity");
		MinOpacity->DefaultValue = 0.2f;
		EditorData->ExpressionCollection.AddExpression(MinOpacity);

		UMaterialExpressionFresnel* Fresnel = NewObject<UMaterialExpressionFresnel>(Material);
		Fresnel->Exponent = 2.4f;
		Fresnel->BaseReflectFraction = 0.f;
		EditorData->ExpressionCollection.AddExpression(Fresnel);
		Fresnel->ExponentIn.Connect(0, Exponent);

		UMaterialExpressionMultiply* RimBoosted = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(RimBoosted);
		RimBoosted->A.Connect(0, Fresnel);
		RimBoosted->B.Connect(0, OpacityBoost);

		UMaterialExpressionAdd* RimPlusFill = NewObject<UMaterialExpressionAdd>(Material);
		EditorData->ExpressionCollection.AddExpression(RimPlusFill);
		RimPlusFill->A.Connect(0, RimBoosted);
		RimPlusFill->B.Connect(0, MinOpacity);
		RimPlusFill->ConnectExpression(&EditorData->Opacity, 0);

		// RGB only — masking .A off a vector param fails to compile on SM6 in UE 5.8.
		UMaterialExpressionComponentMask* ColorRGB = NewObject<UMaterialExpressionComponentMask>(Material);
		ColorRGB->R = true;
		ColorRGB->G = true;
		ColorRGB->B = true;
		ColorRGB->A = false;
		EditorData->ExpressionCollection.AddExpression(ColorRGB);
		ColorRGB->Input.Connect(0, OutlineColor);

		UMaterialExpressionMultiply* Emissive = NewObject<UMaterialExpressionMultiply>(Material);
		EditorData->ExpressionCollection.AddExpression(Emissive);
		Emissive->A.Connect(0, ColorRGB);
		Emissive->B.Connect(0, Intensity);
		Emissive->ConnectExpression(&EditorData->EmissiveColor, 0);

		bool bNeedsRecompile = false;
		Material->SetMaterialUsage(bNeedsRecompile, MATUSAGE_StaticMesh);
		Material->SetMaterialUsage(bNeedsRecompile, MATUSAGE_SkeletalMesh);
		FinalizeMaterial(Material);
		return Material;
	}

	static UMaterialInterface* EnsureContentMaterial(
		const TCHAR* ObjectPath,
		const TCHAR* LongPackageName,
		FName AssetName,
		TFunctionRef<UMaterial*(UObject* Outer, FName Name)> Builder)
	{
		if (UMaterialInterface* Existing = LoadObject<UMaterialInterface>(nullptr, ObjectPath))
		{
			return Existing;
		}

		static TMap<FName, TWeakObjectPtr<UMaterial>> TransientCache;
		if (UMaterial* Cached = TransientCache.FindRef(AssetName).Get())
		{
			return Cached;
		}

#if WITH_EDITOR
		UPackage* Package = CreatePackage(LongPackageName);
		Package->FullyLoad();
		UMaterial* AssetMaterial = Builder(Package, AssetName);
		TrySaveMaterialAsset(AssetMaterial, LongPackageName);
		TransientCache.Add(AssetName, AssetMaterial);
		return AssetMaterial;
#else
		UMaterial* RuntimeMaterial = Builder(GetTransientPackage(), AssetName);
		TransientCache.Add(AssetName, RuntimeMaterial);
		return RuntimeMaterial;
#endif
	}
#endif // WITH_EDITORONLY_DATA

	UMaterialInterface* GetPoisonPulseMaterial()
	{
#if WITH_EDITORONLY_DATA
		return EnsureContentMaterial(
			PoisonPulsePath,
			PoisonPulsePackage,
			TEXT("M_PoisonPulse"),
			[](UObject* Outer, FName Name) { return BuildPoisonPulseMaterial(Outer, Name); });
#else
		return LoadObject<UMaterialInterface>(nullptr, PoisonPulsePath);
#endif
	}

	UMaterialInterface* GetTowerDamageDisplaceMaterial()
	{
#if WITH_EDITORONLY_DATA
		return EnsureContentMaterial(
			TowerDamagePath,
			TowerDamagePackage,
			TEXT("M_TowerDamageDisplace"),
			[](UObject* Outer, FName Name) { return BuildTowerDamageDisplaceMaterial(Outer, Name); });
#else
		return LoadObject<UMaterialInterface>(nullptr, TowerDamagePath);
#endif
	}

	UMaterialInterface* GetTierAuraMaterial()
	{
#if WITH_EDITORONLY_DATA
		return EnsureContentMaterial(
			TierAuraPath,
			TierAuraPackage,
			TEXT("M_TierAuraV2"),
			[](UObject* Outer, FName Name) { return BuildTierAuraMaterial(Outer, Name); });
#else
		return LoadObject<UMaterialInterface>(nullptr, TierAuraPath);
#endif
	}

	UMaterialInterface* GetEnragePulseMaterial()
	{
#if WITH_EDITORONLY_DATA
		return EnsureContentMaterial(
			EnragePulsePath,
			EnragePulsePackage,
			TEXT("M_EnragePulse"),
			[](UObject* Outer, FName Name) { return BuildEnragePulseMaterial(Outer, Name); });
#else
		return LoadObject<UMaterialInterface>(nullptr, EnragePulsePath);
#endif
	}

	UMaterialInterface* GetHoverOutlineMaterial()
	{
#if WITH_EDITORONLY_DATA
		return EnsureContentMaterial(
			HoverOutlinePath,
			HoverOutlinePackage,
			TEXT("M_HoverOutlineV3"),
			[](UObject* Outer, FName Name) { return BuildHoverOutlineMaterial(Outer, Name); });
#else
		return LoadObject<UMaterialInterface>(nullptr, HoverOutlinePath);
#endif
	}

	UMaterialInterface* GetRockMineDisplaceMaterial()
	{
#if WITH_EDITORONLY_DATA
		return EnsureContentMaterial(
			RockMinePath,
			RockMinePackage,
			TEXT("M_RockMineDisplaceV2"),
			[](UObject* Outer, FName Name) { return BuildRockMineDisplaceMaterial(Outer, Name); });
#else
		return LoadObject<UMaterialInterface>(nullptr, RockMinePath);
#endif
	}

	UMaterialInstanceDynamic* CreatePoisonPulseMID(UObject* Outer)
	{
		UMaterialInterface* Parent = GetPoisonPulseMaterial();
		if (!Parent || !Outer)
		{
			return nullptr;
		}
		return UMaterialInstanceDynamic::Create(Parent, Outer);
	}

	UMaterialInstanceDynamic* CreateTowerDamageDisplaceMID(UObject* Outer)
	{
		UMaterialInterface* Parent = GetTowerDamageDisplaceMaterial();
		if (!Parent || !Outer)
		{
			return nullptr;
		}
		return UMaterialInstanceDynamic::Create(Parent, Outer);
	}

	UMaterialInstanceDynamic* CreateTierAuraMID(UObject* Outer)
	{
		UMaterialInterface* Parent = GetTierAuraMaterial();
		if (!Parent || !Outer)
		{
			return nullptr;
		}
		return UMaterialInstanceDynamic::Create(Parent, Outer);
	}

	UMaterialInstanceDynamic* CreateEnragePulseMID(UObject* Outer)
	{
		UMaterialInterface* Parent = GetEnragePulseMaterial();
		if (!Parent || !Outer)
		{
			return nullptr;
		}
		return UMaterialInstanceDynamic::Create(Parent, Outer);
	}

	UMaterialInstanceDynamic* CreateHoverOutlineMID(UObject* Outer)
	{
		UMaterialInterface* Parent = GetHoverOutlineMaterial();
		if (!Parent || !Outer)
		{
			return nullptr;
		}
		return UMaterialInstanceDynamic::Create(Parent, Outer);
	}

	UMaterialInstanceDynamic* CreateRockMineDisplaceMID(UObject* Outer)
	{
		UMaterialInterface* Parent = GetRockMineDisplaceMaterial();
		if (!Parent || !Outer)
		{
			return nullptr;
		}
		return UMaterialInstanceDynamic::Create(Parent, Outer);
	}
}
