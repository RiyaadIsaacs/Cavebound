#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CaveboundDamageVignette.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class APostProcessVolume;
class UWorld;

/**
 * Screen-space radial red damage vignette (post-process), pulsed when the tree is hit.
 */
UCLASS()
class CAVEBOUND_API UCaveboundDamageVignette : public UObject
{
	GENERATED_BODY()

public:
	void Ensure(UWorld* World);
	void Pulse(float Strength = 0.75f, float Duration = 0.55f);
	void TickFade(float DeltaTime);
	bool IsActive() const { return bActive; }

protected:
	void SetStrength(float Alpha);
	void BindToWorld(UWorld* World);
	static UMaterialInterface* GetOrBuildMaterial();

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> VignetteMID;

	UPROPERTY()
	TObjectPtr<APostProcessVolume> Volume;

	float Life = 0.f;
	float Duration = 0.55f;
	float PeakStrength = 0.75f;
	bool bActive = false;
};
