#include "CaveboundDamageFlash.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

static const FName ColorParam(TEXT("Color"));

void FCaveboundDamageFlash::RestoreFlash(TArray<FCaveboundMeshFlashState>& Cache)
{
	for (FCaveboundMeshFlashState& Entry : Cache)
	{
		if (UMaterialInstanceDynamic* MID = Entry.MID.Get())
		{
			MID->SetVectorParameterValue(ColorParam, Entry.OriginalColor);
		}
	}

	Cache.Reset();
}

void FCaveboundDamageFlash::FlashMeshes(
	UObject* WorldContext,
	const TArray<UStaticMeshComponent*>& Meshes,
	FTimerHandle& TimerHandle,
	TArray<FCaveboundMeshFlashState>& Cache,
	FLinearColor FlashColor,
	float Duration)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || Duration <= 0.f)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(TimerHandle);
	RestoreFlash(Cache);

	for (UStaticMeshComponent* Mesh : Meshes)
	{
		if (!IsValid(Mesh))
		{
			continue;
		}

		const int32 NumMaterials = Mesh->GetNumMaterials();
		for (int32 MatIndex = 0; MatIndex < NumMaterials; ++MatIndex)
		{
			UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamic(MatIndex);
			if (!MID)
			{
				continue;
			}

			FLinearColor OriginalColor = FLinearColor::White;
			MID->GetVectorParameterValue(ColorParam, OriginalColor);

			FCaveboundMeshFlashState State;
			State.Mesh = Mesh;
			State.MaterialIndex = MatIndex;
			State.OriginalColor = OriginalColor;
			State.MID = MID;
			Cache.Add(State);

			MID->SetVectorParameterValue(ColorParam, FlashColor);
		}
	}

	if (Cache.Num() == 0)
	{
		return;
	}

	AActor* Owner = Cast<AActor>(WorldContext);
	TArray<FCaveboundMeshFlashState>* CachePtr = &Cache;
	FTimerDelegate RestoreDelegate;
	RestoreDelegate.BindWeakLambda(Owner, [CachePtr]()
	{
		FCaveboundDamageFlash::RestoreFlash(*CachePtr);
	});

	World->GetTimerManager().SetTimer(TimerHandle, RestoreDelegate, Duration, false);
}
