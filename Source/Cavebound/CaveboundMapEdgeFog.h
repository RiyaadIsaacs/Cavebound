#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CaveboundMapEdgeFog.generated.h"

class UMaterialInstanceDynamic;
class APostProcessVolume;
class AExponentialHeightFog;

/** Silent Hill fog: post-process fades the map rim (and void beyond) into fog color. */
UCLASS()
class CAVEBOUND_API ACaveboundMapEdgeFog : public AActor
{
	GENERATED_BODY()

public:
	ACaveboundMapEdgeFog();

	virtual void BeginPlay() override;

	void RefreshFromTerrain();

	/** Call after the map expands so fog retreats with the new rim. */
	void PullBackToExpandedMap();

	/** Clear playable core ends this far inside the terrain rim. */
	UPROPERTY(EditAnywhere, Category = "Fog")
	float EdgeInset = 900.f;

	/** Soft ramp from clear core to full fog. */
	UPROPERTY(EditAnywhere, Category = "Fog")
	float FadeWidth = 1600.f;

	UPROPERTY(EditAnywhere, Category = "Fog")
	float Strength = 1.f;

	UPROPERTY(EditAnywhere, Category = "Fog")
	FLinearColor FogColor = FLinearColor(0.12f, 0.12f, 0.13f, 1.f);

protected:
	void EnsurePostProcess();
	void EnsureHeightFog(const FVector& Center);
	static UMaterialInterface* GetOrBuildFogPPMaterial();

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> FogMID;

	UPROPERTY()
	TObjectPtr<APostProcessVolume> FogVolume;

	UPROPERTY()
	TObjectPtr<AExponentialHeightFog> HeightFog;
};
