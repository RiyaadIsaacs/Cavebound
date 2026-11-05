#include "CaveboundDamageVignette.h"

#include "Engine/PostProcessVolume.h"
#include "Engine/Scene.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Materials/MaterialExpressionScreenPosition.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#endif

namespace
{
	constexpr const TCHAR* VignettePackage = TEXT("/Game/Assets/Materials/PostFX/PP_CaveboundScreenFXV3");
	constexpr const TCHAR* VignettePath = TEXT("/Game/Assets/Materials/PostFX/PP_CaveboundScreenFXV3.PP_CaveboundScreenFXV3");
	constexpr const TCHAR* VolumeTag = TEXT("CaveboundDamageVignette");
	constexpr const TCHAR* StrengthParamName = TEXT("Strength");
	constexpr const TCHAR* DarkenParamName = TEXT("Darken");
	constexpr const TCHAR* TintParamName = TEXT("Tint");

	constexpr float MaxTreeDarken = 0.5f;

#if WITH_EDITORONLY_DATA
	UMaterial* BuildVignetteMaterial(UObject* Outer, FName AssetName)
	{
		UMaterial* Material = NewObject<UMaterial>(Outer, AssetName, RF_Public | RF_Standalone);
		Material->MaterialDomain = MD_PostProcess;
		Material->BlendableLocation = BL_SceneColorAfterTonemapping;
		Material->bAutomaticallySetUsageInEditor = true;

		UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
		if (!EditorData)
		{
			return Material;
		}

		UMaterialExpressionScreenPosition* ScreenPos = NewObject<UMaterialExpressionScreenPosition>(Material);
		EditorData->ExpressionCollection.AddExpression(ScreenPos);

		UMaterialExpressionSceneTexture* SceneColorTex = NewObject<UMaterialExpressionSceneTexture>(Material);
		SceneColorTex->SceneTextureId = PPI_PostProcessInput0;
		SceneColorTex->bFiltered = false;
		EditorData->ExpressionCollection.AddExpression(SceneColorTex);

		UMaterialExpressionScalarParameter* StrengthParam = NewObject<UMaterialExpressionScalarParameter>(Material);
		StrengthParam->ParameterName = StrengthParamName;
		StrengthParam->DefaultValue = 0.f;
		EditorData->ExpressionCollection.AddExpression(StrengthParam);

		UMaterialExpressionScalarParameter* DarkenParam = NewObject<UMaterialExpressionScalarParameter>(Material);
		DarkenParam->ParameterName = DarkenParamName;
		DarkenParam->DefaultValue = 0.f;
		EditorData->ExpressionCollection.AddExpression(DarkenParam);

		UMaterialExpressionVectorParameter* TintParam = NewObject<UMaterialExpressionVectorParameter>(Material);
		TintParam->ParameterName = TintParamName;
		TintParam->DefaultValue = FLinearColor(0.75f, 0.05f, 0.05f, 1.f);
		EditorData->ExpressionCollection.AddExpression(TintParam);

		UMaterialExpressionCustom* Custom = NewObject<UMaterialExpressionCustom>(Material);
		Custom->Description = TEXT("CaveboundScreenFXV3");
		Custom->OutputType = CMOT_Float3;
		Custom->Code = TEXT(
			"float3 SceneColor = SceneColorRef.rgb;\n"
			"float Dark = saturate(Darken);\n"
			"SceneColor *= (1.0 - Dark);\n"
			"float Amt = saturate(Strength);\n"
			"if (Amt <= 0.001)\n"
			"{\n"
			"	return SceneColor;\n"
			"}\n"
			"float2 ViewportUV = UV.xy;\n"
			"float2 C = ViewportUV * 2.0 - 1.0;\n"
			"float Dist = length(C);\n"
			"float V = saturate(smoothstep(0.30, 1.15, Dist));\n"
			"V = pow(V, 1.4);\n"
			"float3 EdgeCol = saturate(Tint.rgb);\n"
			"float3 Edge = SceneColor * (EdgeCol * 0.35) + EdgeCol * 0.75;\n"
			"return lerp(SceneColor, Edge, saturate(V * Amt));\n");
		EditorData->ExpressionCollection.AddExpression(Custom);

		{
			FCustomInput& In = Custom->Inputs.AddDefaulted_GetRef();
			In.InputName = TEXT("UV");
			ScreenPos->ConnectExpression(&In.Input, 0);
		}
		{
			FCustomInput& In = Custom->Inputs.AddDefaulted_GetRef();
			In.InputName = TEXT("SceneColorRef");
			SceneColorTex->ConnectExpression(&In.Input, 0);
		}
		{
			FCustomInput& In = Custom->Inputs.AddDefaulted_GetRef();
			In.InputName = TEXT("Strength");
			StrengthParam->ConnectExpression(&In.Input, 0);
		}
		{
			FCustomInput& In = Custom->Inputs.AddDefaulted_GetRef();
			In.InputName = TEXT("Darken");
			DarkenParam->ConnectExpression(&In.Input, 0);
		}
		{
			FCustomInput& In = Custom->Inputs.AddDefaulted_GetRef();
			In.InputName = TEXT("Tint");
			TintParam->ConnectExpression(&In.Input, 0);
		}

		Custom->ConnectExpression(&EditorData->EmissiveColor, 0);

		Material->PreEditChange(nullptr);
		Material->PostEditChange();
		return Material;
	}

#if WITH_EDITOR
	void TrySaveMaterialAsset(UMaterial* Material, const TCHAR* LongPackageName)
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
#endif // WITH_EDITORONLY_DATA
}

UMaterialInterface* UCaveboundDamageVignette::GetOrBuildMaterial()
{
	if (UMaterialInterface* Existing = LoadObject<UMaterialInterface>(nullptr, VignettePath))
	{
		return Existing;
	}

#if WITH_EDITORONLY_DATA
	static TWeakObjectPtr<UMaterial> TransientCache;
	if (UMaterial* Cached = TransientCache.Get())
	{
		return Cached;
	}

#if WITH_EDITOR
	UPackage* Package = CreatePackage(VignettePackage);
	Package->FullyLoad();
	UMaterial* AssetMaterial = BuildVignetteMaterial(Package, TEXT("PP_CaveboundScreenFXV3"));
	TrySaveMaterialAsset(AssetMaterial, VignettePackage);
	TransientCache = AssetMaterial;
	return AssetMaterial;
#else
	UMaterial* RuntimeMaterial = BuildVignetteMaterial(GetTransientPackage(), TEXT("PP_CaveboundScreenFXV3"));
	TransientCache = RuntimeMaterial;
	return RuntimeMaterial;
#endif
#else
	return nullptr;
#endif
}

void UCaveboundDamageVignette::BindToWorld(UWorld* World)
{
	if (!World || !VignetteMID)
	{
		return;
	}

	if (!IsValid(Volume))
	{
		for (TActorIterator<APostProcessVolume> It(World); It; ++It)
		{
			APostProcessVolume* Existing = *It;
			if (Existing && Existing->ActorHasTag(VolumeTag))
			{
				Volume = Existing;
				break;
			}
		}
	}

	if (!IsValid(Volume))
	{
		FActorSpawnParameters Params;
		Params.Name = TEXT("CaveboundDamageVignetteVolume");
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Volume = World->SpawnActor<APostProcessVolume>(
			APostProcessVolume::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			Params);
		if (Volume)
		{
			Volume->Tags.AddUnique(VolumeTag);
		}
	}

	if (IsValid(Volume))
	{
		Volume->bUnbound = true;
		Volume->Priority = 120.f;
		Volume->BlendWeight = 1.f;
		Volume->bEnabled = true;

		FPostProcessSettings& Settings = Volume->Settings;
		Settings.WeightedBlendables.Array.Reset();
		FWeightedBlendable Blend;
		Blend.Object = VignetteMID;
		Blend.Weight = 1.f;
		Settings.WeightedBlendables.Array.Add(Blend);
	}
}

void UCaveboundDamageVignette::Ensure(UWorld* World)
{
	if (!VignetteMID)
	{
		if (UMaterialInterface* Base = GetOrBuildMaterial())
		{
			VignetteMID = UMaterialInstanceDynamic::Create(Base, this);
			if (VignetteMID)
			{
				ApplyVignetteParams(0.f);
				ApplyDarken();
			}
		}
	}

	BindToWorld(World);
	if (bActive)
	{
		const float T = Life / FMath::Max(Duration, 0.05f);
		ApplyVignetteParams(PeakStrength * FMath::Clamp(T, 0.f, 1.f));
	}
	else
	{
		ApplyVignetteParams(0.f);
	}
	ApplyDarken();
}

void UCaveboundDamageVignette::ApplyVignetteParams(float Strength)
{
	if (!VignetteMID)
	{
		return;
	}
	VignetteMID->SetScalarParameterValue(StrengthParamName, FMath::Clamp(Strength, 0.f, 1.f));
	VignetteMID->SetVectorParameterValue(TintParamName, ActiveTint);
}

void UCaveboundDamageVignette::ApplyDarken()
{
	if (VignetteMID)
	{
		VignetteMID->SetScalarParameterValue(DarkenParamName, FMath::Clamp(DarkenAmount, 0.f, MaxTreeDarken));
	}
}

void UCaveboundDamageVignette::Pulse(
	ECaveboundVignetteKind Kind,
	const FLinearColor& Tint,
	float Strength,
	float InDuration)
{
	// Red damage outranks collect pulses while it is still fading.
	if (bActive
		&& ActiveKind == ECaveboundVignetteKind::Damage
		&& Kind != ECaveboundVignetteKind::Damage)
	{
		return;
	}

	ActiveKind = Kind;
	ActiveTint = Tint;
	PeakStrength = FMath::Clamp(Strength, 0.f, 1.f);
	Duration = FMath::Max(0.05f, InDuration);
	Life = Duration;
	bActive = true;
	ApplyVignetteParams(PeakStrength);
}

void UCaveboundDamageVignette::PulseDamage(float Strength, float InDuration)
{
	Pulse(ECaveboundVignetteKind::Damage, FLinearColor(0.85f, 0.05f, 0.05f, 1.f), Strength, InDuration);
}

void UCaveboundDamageVignette::PulseWood(float Strength, float InDuration)
{
	Pulse(ECaveboundVignetteKind::Wood, FLinearColor(0.12f, 0.85f, 0.22f, 1.f), Strength, InDuration);
}

void UCaveboundDamageVignette::PulseRock(float Strength, float InDuration)
{
	Pulse(ECaveboundVignetteKind::Rock, FLinearColor(0.62f, 0.62f, 0.66f, 1.f), Strength, InDuration);
}

void UCaveboundDamageVignette::SetTreeHealthFraction(float HealthFrac)
{
	const float Clamped = FMath::Clamp(HealthFrac, 0.f, 1.f);
	DarkenAmount = (1.f - Clamped) * MaxTreeDarken;
	ApplyDarken();
}

void UCaveboundDamageVignette::TickFade(float DeltaTime)
{
	if (!bActive)
	{
		return;
	}

	Life -= DeltaTime;
	if (Life <= 0.f)
	{
		bActive = false;
		ActiveKind = ECaveboundVignetteKind::None;
		ApplyVignetteParams(0.f);
		return;
	}

	const float T = Life / Duration;
	ApplyVignetteParams(PeakStrength * FMath::InterpEaseIn(0.f, 1.f, T, 2.f));
}
