#include "CaveboundMapEdgeFog.h"

#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#endif

#if WITH_EDITORONLY_DATA
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#endif

namespace
{
	const FName FogVolumeTag(TEXT("CaveboundMapEdgeFogPP"));
	constexpr const TCHAR* FogPPPackage = TEXT("/Game/Assets/Materials/PP_CaveboundMapFog");
	constexpr const TCHAR* FogPPPath = TEXT("/Game/Assets/Materials/PP_CaveboundMapFog.PP_CaveboundMapFog");

	bool ReadFloatProp(AActor* Actor, const TCHAR* Name, float& OutValue)
	{
		if (!Actor)
		{
			return false;
		}
		if (FNumericProperty* Prop = FindFProperty<FNumericProperty>(Actor->GetClass(), Name))
		{
			OutValue = static_cast<float>(Prop->GetFloatingPointPropertyValue(Prop->ContainerPtrToValuePtr<void>(Actor)));
			return true;
		}
		return false;
	}

	bool ReadIntProp(AActor* Actor, const TCHAR* Name, int32& OutValue)
	{
		if (!Actor)
		{
			return false;
		}
		if (FIntProperty* Prop = FindFProperty<FIntProperty>(Actor->GetClass(), Name))
		{
			OutValue = Prop->GetPropertyValue_InContainer(Actor);
			return true;
		}
		return false;
	}

	AActor* FindProceduralTerrain(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		UClass* TerrainClass = LoadClass<AActor>(
			nullptr,
			TEXT("/Game/Blueprints/BP_ProceduralTerrain.BP_ProceduralTerrain_C"));
		if (!TerrainClass)
		{
			return nullptr;
		}

		TArray<AActor*> Terrains;
		UGameplayStatics::GetAllActorsOfClass(World, TerrainClass, Terrains);
		return Terrains.Num() > 0 ? Terrains[0] : nullptr;
	}

#if WITH_EDITORONLY_DATA
	UMaterial* BuildMapFogPPMaterial(UObject* Outer, FName AssetName)
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

		UMaterialExpressionSceneTexture* SceneColorTex = NewObject<UMaterialExpressionSceneTexture>(Material);
		SceneColorTex->SceneTextureId = PPI_PostProcessInput0;
		SceneColorTex->bFiltered = false;
		EditorData->ExpressionCollection.AddExpression(SceneColorTex);

		UMaterialExpressionWorldPosition* WorldPos = NewObject<UMaterialExpressionWorldPosition>(Material);
		EditorData->ExpressionCollection.AddExpression(WorldPos);

		UMaterialExpressionVectorParameter* MapCenter = NewObject<UMaterialExpressionVectorParameter>(Material);
		MapCenter->ParameterName = TEXT("MapCenter");
		MapCenter->DefaultValue = FLinearColor(0.f, 0.f, 0.f, 0.f);
		EditorData->ExpressionCollection.AddExpression(MapCenter);

		UMaterialExpressionVectorParameter* HalfExtent = NewObject<UMaterialExpressionVectorParameter>(Material);
		HalfExtent->ParameterName = TEXT("HalfExtent");
		HalfExtent->DefaultValue = FLinearColor(1500.f, 1500.f, 0.f, 0.f);
		EditorData->ExpressionCollection.AddExpression(HalfExtent);

		UMaterialExpressionScalarParameter* FadeWidth = NewObject<UMaterialExpressionScalarParameter>(Material);
		FadeWidth->ParameterName = TEXT("FadeWidth");
		FadeWidth->DefaultValue = 1600.f;
		EditorData->ExpressionCollection.AddExpression(FadeWidth);

		UMaterialExpressionScalarParameter* Strength = NewObject<UMaterialExpressionScalarParameter>(Material);
		Strength->ParameterName = TEXT("Strength");
		Strength->DefaultValue = 1.f;
		EditorData->ExpressionCollection.AddExpression(Strength);

		UMaterialExpressionVectorParameter* FogColor = NewObject<UMaterialExpressionVectorParameter>(Material);
		FogColor->ParameterName = TEXT("FogColor");
		FogColor->DefaultValue = FLinearColor(0.12f, 0.12f, 0.13f, 1.f);
		EditorData->ExpressionCollection.AddExpression(FogColor);

		UMaterialExpressionCustom* Custom = NewObject<UMaterialExpressionCustom>(Material);
		Custom->Description = TEXT("MapEdgeFogPP");
		Custom->OutputType = CMOT_Float3;
		Custom->Code = TEXT(
			"float3 SceneColor = SceneColorRef.rgb;\n"
			"float2 Center = MapCenter.xy;\n"
			"float2 Half = max(HalfExtent.xy, float2(1.0, 1.0));\n"
			"float2 P = abs(WorldPos.xy - Center) - Half;\n"
			"float Sd = length(max(P, 0.0)) + min(max(P.x, P.y), 0.0);\n"
			"float Fade = max(FadeWidth, 1.0);\n"
			"float FogAmt = saturate(Sd / Fade);\n"
			"FogAmt = smoothstep(0.0, 1.0, FogAmt);\n"
			"FogAmt = 1.0 - pow(1.0 - FogAmt, 2.2);\n"
			"FogAmt *= saturate(Strength);\n"
			"float3 FogCol = saturate(FogColor.rgb);\n"
			"return lerp(SceneColor, FogCol, FogAmt);\n");
		EditorData->ExpressionCollection.AddExpression(Custom);

		{
			FCustomInput& In = Custom->Inputs.AddDefaulted_GetRef();
			In.InputName = TEXT("SceneColorRef");
			SceneColorTex->ConnectExpression(&In.Input, 0);
		}
		{
			FCustomInput& In = Custom->Inputs.AddDefaulted_GetRef();
			In.InputName = TEXT("WorldPos");
			WorldPos->ConnectExpression(&In.Input, 0);
		}
		{
			FCustomInput& In = Custom->Inputs.AddDefaulted_GetRef();
			In.InputName = TEXT("MapCenter");
			MapCenter->ConnectExpression(&In.Input, 0);
		}
		{
			FCustomInput& In = Custom->Inputs.AddDefaulted_GetRef();
			In.InputName = TEXT("HalfExtent");
			HalfExtent->ConnectExpression(&In.Input, 0);
		}
		{
			FCustomInput& In = Custom->Inputs.AddDefaulted_GetRef();
			In.InputName = TEXT("FadeWidth");
			FadeWidth->ConnectExpression(&In.Input, 0);
		}
		{
			FCustomInput& In = Custom->Inputs.AddDefaulted_GetRef();
			In.InputName = TEXT("Strength");
			Strength->ConnectExpression(&In.Input, 0);
		}
		{
			FCustomInput& In = Custom->Inputs.AddDefaulted_GetRef();
			In.InputName = TEXT("FogColor");
			FogColor->ConnectExpression(&In.Input, 0);
		}

		Custom->ConnectExpression(&EditorData->EmissiveColor, 0);

		Material->PreEditChange(nullptr);
		Material->PostEditChange();
		return Material;
	}
#endif
}

ACaveboundMapEdgeFog::ACaveboundMapEdgeFog()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);
}

UMaterialInterface* ACaveboundMapEdgeFog::GetOrBuildFogPPMaterial()
{
	if (UMaterialInterface* Existing = LoadObject<UMaterialInterface>(nullptr, FogPPPath))
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
	UPackage* Package = CreatePackage(FogPPPackage);
	Package->FullyLoad();
	UMaterial* AssetMaterial = BuildMapFogPPMaterial(Package, TEXT("PP_CaveboundMapFog"));
	if (AssetMaterial)
	{
		Package->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(AssetMaterial);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			FogPPPackage,
			FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		UPackage::SavePackage(Package, AssetMaterial, *Filename, SaveArgs);
	}
	TransientCache = AssetMaterial;
	return AssetMaterial;
#else
	UMaterial* RuntimeMaterial = BuildMapFogPPMaterial(GetTransientPackage(), TEXT("PP_CaveboundMapFog"));
	TransientCache = RuntimeMaterial;
	return RuntimeMaterial;
#endif
#else
	return nullptr;
#endif
}

void ACaveboundMapEdgeFog::EnsurePostProcess()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!FogMID)
	{
		if (UMaterialInterface* Base = GetOrBuildFogPPMaterial())
		{
			FogMID = UMaterialInstanceDynamic::Create(Base, this);
		}
	}

	if (!FogMID)
	{
		return;
	}

	if (!IsValid(FogVolume))
	{
		for (TActorIterator<APostProcessVolume> It(World); It; ++It)
		{
			APostProcessVolume* Existing = *It;
			if (Existing && Existing->ActorHasTag(FogVolumeTag))
			{
				FogVolume = Existing;
				break;
			}
		}
	}

	if (!IsValid(FogVolume))
	{
		FActorSpawnParameters Params;
		Params.Name = TEXT("CaveboundMapEdgeFogVolume");
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FogVolume = World->SpawnActor<APostProcessVolume>(
			APostProcessVolume::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			Params);
		if (FogVolume)
		{
			FogVolume->Tags.AddUnique(FogVolumeTag);
		}
	}

	if (!IsValid(FogVolume))
	{
		return;
	}

	FogVolume->bUnbound = true;
	FogVolume->Priority = 50.f; // below damage vignette (120), always on
	FogVolume->BlendWeight = 1.f;
	FogVolume->bEnabled = true;

	FPostProcessSettings& Settings = FogVolume->Settings;
	bool bAlreadyBound = false;
	for (const FWeightedBlendable& Blend : Settings.WeightedBlendables.Array)
	{
		if (Blend.Object == FogMID)
		{
			bAlreadyBound = true;
			break;
		}
	}
	if (!bAlreadyBound)
	{
		FWeightedBlendable Blend;
		Blend.Object = FogMID;
		Blend.Weight = 1.f;
		Settings.WeightedBlendables.Array.Add(Blend);
	}
}

void ACaveboundMapEdgeFog::EnsureHeightFog(const FVector& Center)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!HeightFog)
	{
		TArray<AActor*> Existing;
		UGameplayStatics::GetAllActorsOfClass(World, AExponentialHeightFog::StaticClass(), Existing);
		if (Existing.Num() > 0)
		{
			HeightFog = Cast<AExponentialHeightFog>(Existing[0]);
		}
	}

	if (!HeightFog)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		HeightFog = World->SpawnActor<AExponentialHeightFog>(
			AExponentialHeightFog::StaticClass(),
			Center,
			FRotator::ZeroRotator,
			Params);
	}

	if (!HeightFog)
	{
		return;
	}

	HeightFog->SetActorLocation(Center);
	if (UExponentialHeightFogComponent* Comp = HeightFog->GetComponent())
	{
		Comp->SetFogDensity(0.05f);
		Comp->SetFogHeightFalloff(0.18f);
		Comp->SetFogInscatteringColor(FogColor);
		Comp->SetFogMaxOpacity(1.f);
		Comp->SetStartDistance(0.f);
		Comp->bEnableVolumetricFog = true;
		Comp->VolumetricFogExtinctionScale = 1.5f;
		Comp->MarkRenderStateDirty();
	}
}

void ACaveboundMapEdgeFog::BeginPlay()
{
	Super::BeginPlay();
	EnsurePostProcess();
	RefreshFromTerrain();
}

void ACaveboundMapEdgeFog::RefreshFromTerrain()
{
	EnsurePostProcess();

	UWorld* World = GetWorld();
	AActor* Terrain = FindProceduralTerrain(World);
	if (!Terrain || !FogMID)
	{
		return;
	}

	float HalfWidth = 0.f;
	float HalfHeight = 0.f;
	ReadFloatProp(Terrain, TEXT("HalfWidth"), HalfWidth);
	ReadFloatProp(Terrain, TEXT("HalfHeight"), HalfHeight);

	if (HalfWidth <= 1.f || HalfHeight <= 1.f)
	{
		int32 GridX = 0;
		int32 GridY = 0;
		float CellSize = 100.f;
		ReadIntProp(Terrain, TEXT("GridSizeX"), GridX);
		ReadIntProp(Terrain, TEXT("GridSizeY"), GridY);
		ReadFloatProp(Terrain, TEXT("CellSize"), CellSize);
		HalfWidth = GridX * CellSize * 0.5f;
		HalfHeight = GridY * CellSize * 0.5f;
	}

	if (HalfWidth <= 1.f || HalfHeight <= 1.f)
	{
		HalfWidth = 2000.f;
		HalfHeight = 2000.f;
	}

	const FVector Center = Terrain->GetActorLocation();
	const float ClearHalfX = FMath::Max(300.f, HalfWidth - EdgeInset);
	const float ClearHalfY = FMath::Max(300.f, HalfHeight - EdgeInset);

	FogMID->SetVectorParameterValue(TEXT("MapCenter"), FLinearColor(Center.X, Center.Y, Center.Z, 0.f));
	FogMID->SetVectorParameterValue(TEXT("HalfExtent"), FLinearColor(ClearHalfX, ClearHalfY, 0.f, 0.f));
	FogMID->SetScalarParameterValue(TEXT("FadeWidth"), FadeWidth);
	FogMID->SetScalarParameterValue(TEXT("Strength"), Strength);
	FogMID->SetVectorParameterValue(TEXT("FogColor"), FogColor);

	EnsureHeightFog(Center);
}
