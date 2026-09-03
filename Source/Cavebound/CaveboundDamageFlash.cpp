#include "CaveboundDamageFlash.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

// A flat unlit red material built from the engine's WidgetMaterial_Transparent
static UMaterialInstanceDynamic* GetOrCreateRedMID(UObject* WorldContext)
{
	// One shared MID across the whole session
	static TWeakObjectPtr<UMaterialInstanceDynamic> SharedMID;
	if (UMaterialInstanceDynamic* Existing = SharedMID.Get())
	{
		return Existing;
	}

	// Use the engine's widget translucent material as a base
	UMaterialInterface* Base = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(),nullptr,
		TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent")));

	if (!Base)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, WorldContext);
	if (MID)
	{
		// Bright red, slightly transparent so the original shape still reads through
		MID->SetVectorParameterValue(FName(TEXT("Color")), FLinearColor(1.f, 0.f, 0.f, 0.7f));
		SharedMID = MID;
	}

	return MID;
}

void FCaveboundDamageFlash::ClearFlash(const TArray<UStaticMeshComponent*>& Meshes)
{
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		if (IsValid(Mesh))
		{
			Mesh->SetOverlayMaterial(nullptr);
		}
	}
}

void FCaveboundDamageFlash::FlashMeshes(UObject* WorldContext,const TArray<UStaticMeshComponent*>& Meshes,FTimerHandle& TimerHandle,
	FLinearColor FlashColor,
	float Duration)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || Duration <= 0.f)
	{
		return;
	}

	// Clear any previous flash timer so rapid hits restart cleanly
	World->GetTimerManager().ClearTimer(TimerHandle);

	UMaterialInstanceDynamic* RedMID = GetOrCreateRedMID(WorldContext);
	if (!RedMID)
	{
		return;
	}

	// FlashColor defaults to red but allow overrides
	RedMID->SetVectorParameterValue(FName(TEXT("Color")), FlashColor);

	// Apply the overlay to every mesh
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		if (IsValid(Mesh))
		{
			Mesh->SetOverlayMaterial(RedMID);
		}
	}

	// After Duration, remove the overlay
	AActor* Owner = Cast<AActor>(WorldContext);
	TArray<UStaticMeshComponent*> MeshesCopy = Meshes;
	FTimerDelegate RestoreDelegate;
	RestoreDelegate.BindWeakLambda(Owner, [MeshesCopy]()
	{
		for (UStaticMeshComponent* Mesh : MeshesCopy)
		{
			if (IsValid(Mesh))
			{
				Mesh->SetOverlayMaterial(nullptr);
			}
		}
	});

	World->GetTimerManager().SetTimer(TimerHandle, RestoreDelegate, Duration, false);
}
