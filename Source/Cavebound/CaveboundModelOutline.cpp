#include "CaveboundModelOutline.h"

#include "CaveboundBaseEnemy.h"
#include "CaveboundCharacter.h"
#include "CaveboundRockDeposit.h"
#include "CaveboundTree.h"
#include "CaveboundTurret.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "Editor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogCaveboundModelOutline, Log, All);

namespace CaveboundModelOutline
{
	constexpr const TCHAR* OutlinePath = TEXT("/Game/Assets/Materials/PostFX/M_Outline.M_Outline");

#if WITH_EDITOR
	static void EnsureUsageFlagsAndShaders(UMaterial* Base)
	{
		if (!Base)
		{
			return;
		}

		const bool bNeedSkel = !Base->GetUsageByFlag(MATUSAGE_SkeletalMesh);
		const bool bNeedStatic = !Base->GetUsageByFlag(MATUSAGE_StaticMesh);
		if (!bNeedSkel && !bNeedStatic)
		{
			return;
		}

		// SetMaterialUsage() refuses to write flags during PIE — set them directly.
		if (bNeedSkel)
		{
			Base->SetUsageByFlag(MATUSAGE_SkeletalMesh, true);
		}
		if (bNeedStatic)
		{
			Base->SetUsageByFlag(MATUSAGE_StaticMesh, true);
		}
		Base->ForceRecompileForRendering();
		Base->MarkPackageDirty();

		UE_LOG(
			LogCaveboundModelOutline,
			Log,
			TEXT("Enabled skeletal/static usage on %s (skel=%d static=%d)"),
			*Base->GetPathName(),
			Base->GetUsageByFlag(MATUSAGE_SkeletalMesh) ? 1 : 0,
			Base->GetUsageByFlag(MATUSAGE_StaticMesh) ? 1 : 0);

		// Persist outside PIE so the next Play doesn't need a recompile.
		const bool bInPIE = GEditor && GEditor->IsPlayingSessionInEditor();
		if (!bInPIE)
		{
			if (UPackage* Package = Base->GetOutermost())
			{
				FAssetRegistryModule::AssetCreated(Base);
				const FString Filename = FPackageName::LongPackageNameToFilename(
					Package->GetName(),
					FPackageName::GetAssetPackageExtension());
				FSavePackageArgs SaveArgs;
				SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
				UPackage::SavePackage(Package, Base, *Filename, SaveArgs);
			}
		}
	}
#endif

	UMaterialInterface* GetMaterial()
	{
		static TWeakObjectPtr<UMaterialInterface> Cached;
		if (UMaterialInterface* Existing = Cached.Get())
		{
			return Existing;
		}

		UMaterialInterface* Loaded = LoadObject<UMaterialInterface>(nullptr, OutlinePath);
		if (!Loaded)
		{
			UE_LOG(LogCaveboundModelOutline, Error, TEXT("Failed to load %s"), OutlinePath);
			return nullptr;
		}

#if WITH_EDITOR
		if (UMaterial* Base = Loaded->GetMaterial())
		{
			EnsureUsageFlagsAndShaders(Base);
		}
#else
		Loaded->CheckMaterialUsage(MATUSAGE_SkeletalMesh);
		Loaded->CheckMaterialUsage(MATUSAGE_StaticMesh);
#endif

		if (UMaterial* Base = Loaded->GetMaterial())
		{
			if (!Base->GetUsageByFlag(MATUSAGE_SkeletalMesh))
			{
				UE_LOG(
					LogCaveboundModelOutline,
					Warning,
					TEXT("%s still missing SkeletalMesh usage — skeletal overlays will be stripped"),
					*Base->GetPathName());
			}
		}

		Cached = Loaded;
		return Loaded;
	}

	static bool IsRenderableMesh(UMeshComponent* Mesh)
	{
		if (!IsValid(Mesh) || Mesh->bHiddenInGame || !Mesh->GetVisibleFlag())
		{
			return false;
		}
		if (const UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Mesh))
		{
			return StaticMesh->GetStaticMesh() != nullptr;
		}
		if (const USkeletalMeshComponent* Skel = Cast<USkeletalMeshComponent>(Mesh))
		{
			return Skel->GetSkeletalMeshAsset() != nullptr;
		}
		return false;
	}

	void ApplyToMesh(UMeshComponent* Mesh)
	{
		if (!IsRenderableMesh(Mesh))
		{
			return;
		}
		if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Mesh))
		{
			StaticMesh->bDisallowNanite = true;
		}
		if (UMaterialInterface* Outline = GetMaterial())
		{
			Mesh->SetOverlayMaterial(Outline);
		}
	}

	void ApplyToActor(AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return;
		}
		TArray<UMeshComponent*> Meshes;
		Actor->GetComponents<UMeshComponent>(Meshes);
		for (UMeshComponent* Mesh : Meshes)
		{
			ApplyToMesh(Mesh);
		}
	}

	void RestoreMesh(UMeshComponent* Mesh)
	{
		ApplyToMesh(Mesh);
	}

	void RestoreActor(AActor* Actor)
	{
		ApplyToActor(Actor);
	}

	static void ApplyToClass(UWorld* World, UClass* Class)
	{
		if (!World || !Class)
		{
			return;
		}
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(World, Class, Actors);
		for (AActor* Actor : Actors)
		{
			ApplyToActor(Actor);
		}
	}

	void EnsureWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GetMaterial();
		ApplyToClass(World, ACaveboundTree::StaticClass());
		ApplyToClass(World, ACaveboundCharacter::StaticClass());
		ApplyToClass(World, ACaveboundTurret::StaticClass());
		ApplyToClass(World, ACaveboundBaseEnemy::StaticClass());
		ApplyToClass(World, ACaveboundRockDeposit::StaticClass());
		if (UClass* SlotClass = LoadClass<AActor>(nullptr, TEXT("/Game/Blueprints/BP_BuildSlot.BP_BuildSlot_C")))
		{
			ApplyToClass(World, SlotClass);
		}
	}
}
