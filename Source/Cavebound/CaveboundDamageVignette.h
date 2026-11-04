#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaveboundDamageVignette.generated.h"

class UImage;
class UTexture2D;

/**
 * Full-screen soft radial red vignette (real circle falloff, not edge bars).
 * Call Pulse() when the tree takes damage.
 */
UCLASS()
class CAVEBOUND_API UCaveboundDamageVignette : public UUserWidget
{
	GENERATED_BODY()

public:
	void Pulse(float Strength = 0.75f, float Duration = 0.55f);
	void TickFade(float DeltaTime);
	bool IsActive() const { return bActive; }

protected:
	virtual void NativeOnInitialized() override;

	void SetStrength(float Alpha);
	static UTexture2D* GetRadialTexture();

	UPROPERTY()
	TObjectPtr<UImage> VignetteImage;

	float Life = 0.f;
	float Duration = 0.55f;
	float PeakStrength = 0.75f;
	bool bActive = false;
};
