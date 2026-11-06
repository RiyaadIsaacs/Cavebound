#include "CaveboundDamageFlash.h"
#include "CaveboundModelOutline.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

// shared flat translucent MID used for hit flashes
static UMaterialInstanceDynamic* GetOrCreateRedMID(UObject* WorldContext)
{
	static TWeakObjectPtr<UMaterialInstanceDynamic> SharedMID;
	if (UMaterialInstanceDynamic* Existing = SharedMID.Get())
	{
		return Existing;
	}

	UMaterialInterface* Base = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(),nullptr,
		TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent")));

	if (!Base)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, WorldContext);
	if (MID)
	{
		MID->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(1.f, 0.f, 0.f, 0.7f));
		SharedMID = MID;
	}

	return MID;
}

// restores outline after a flash ends
void FCaveboundDamageFlash::ClearFlash(const TArray<UStaticMeshComponent*>& Meshes)
{
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		CaveboundModelOutline::RestoreMesh(Mesh);
	}
}

// flashes multiple static meshes, then restores outline
void FCaveboundDamageFlash::FlashMeshes(UObject* WorldContext,const TArray<UStaticMeshComponent*>& Meshes,FTimerHandle& TimerHandle,
	FLinearColor FlashColor,
	float Duration)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || Duration <= 0.f)
	{
		return;
	}

	// clears previous flash so rapid hits restart cleanly
	World->GetTimerManager().ClearTimer(TimerHandle);

	UMaterialInstanceDynamic* RedMID = GetOrCreateRedMID(WorldContext);
	if (!RedMID)
	{
		return;
	}

	RedMID->SetVectorParameterValue(FName(TEXT("Color")), FlashColor);

	// applies the overlay to every mesh
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		if (IsValid(Mesh))
		{
			Mesh->SetOverlayMaterial(RedMID);
		}
	}

	// schedules restore after Duration
	AActor* Owner = Cast<AActor>(WorldContext);
	TArray<UStaticMeshComponent*> MeshesCopy = Meshes;
	FTimerDelegate RestoreDelegate;
	RestoreDelegate.BindWeakLambda(Owner, [MeshesCopy]()
	{
		for (UStaticMeshComponent* Mesh : MeshesCopy)
		{
			CaveboundModelOutline::RestoreMesh(Mesh);
		}
	});

	World->GetTimerManager().SetTimer(TimerHandle, RestoreDelegate, Duration, false);
}

// flashes a single mesh component, then restores outline
void FCaveboundDamageFlash::FlashComponent(UObject* WorldContext, UMeshComponent* Mesh, FTimerHandle& TimerHandle, FLinearColor FlashColor, float Duration)
{
	UWorld* world = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!world || !IsValid(Mesh) || Duration <= 0.f)
	{
		return;
	}

	world->GetTimerManager().ClearTimer(TimerHandle);

	UMaterialInstanceDynamic* flashMaterial = GetOrCreateRedMID(WorldContext);
	if (!flashMaterial)
	{
		return;
	}

	flashMaterial->SetVectorParameterValue(FName(TEXT("Color")), FlashColor);
	Mesh->SetOverlayMaterial(flashMaterial);

	AActor* ownerActor = Cast<AActor>(WorldContext);
	TWeakObjectPtr<UMeshComponent> meshToFlash = Mesh;
	FTimerDelegate clearFlashLater;
	clearFlashLater.BindWeakLambda(ownerActor, [meshToFlash]()
	{
		CaveboundModelOutline::RestoreMesh(meshToFlash.Get());
	});

	world->GetTimerManager().SetTimer(TimerHandle, clearFlashLater, Duration, false);
}
