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
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSine.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#endif

namespace CaveboundShaderMaterials
{
	static constexpr const TCHAR* PoisonPulsePackage = TEXT("/Game/Assets/Materials/M_PoisonPulse");
	static constexpr const TCHAR* PoisonPulsePath = TEXT("/Game/Assets/Materials/M_PoisonPulse.M_PoisonPulse");
	static constexpr const TCHAR* TowerDamagePackage = TEXT("/Game/Assets/Materials/M_TowerDamageDisplace");
	static constexpr const TCHAR* TowerDamagePath = TEXT("/Game/Assets/Materials/M_TowerDamageDisplace.M_TowerDamageDisplace");
	static constexpr const TCHAR* TowerTexturePath = TEXT("/Game/Assets/Models/Towers/T_FortressElves.T_FortressElves");

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
}
