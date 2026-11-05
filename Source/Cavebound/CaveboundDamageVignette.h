#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CaveboundDamageVignette.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class APostProcessVolume;
class UWorld;

UENUM()
enum class ECaveboundVignetteKind : uint8
{
	None,
	Wood,
	Rock,
	Damage
};

/**
 * Screen-space post FX: colored radial vignettes + tree-health darken.
 * Damage (red) outranks wood (green) / rock (grey) while active.
 */
UCLASS()
class CAVEBOUND_API UCaveboundDamageVignette : public UObject
{
	GENERATED_BODY()

public:
	void Ensure(UWorld* World);

	void PulseDamage(float Strength = 0.8f, float Duration = 0.6f);
	void PulseWood(float Strength = 0.55f, float Duration = 0.35f);
	void PulseRock(float Strength = 0.55f, float Duration = 0.35f);

	/** HealthFrac 1 = full HP, 0 = dead. Darken = (1 - HealthFrac) * 0.5. */
	void SetTreeHealthFraction(float HealthFrac);

	void TickFade(float DeltaTime);
	bool IsDamageActive() const { return bActive && ActiveKind == ECaveboundVignetteKind::Damage; }

protected:
	void Pulse(ECaveboundVignetteKind Kind, const FLinearColor& Tint, float Strength, float Duration);
	void ApplyVignetteParams(float Strength);
	void ApplyDarken();
	void BindToWorld(UWorld* World);
	static UMaterialInterface* GetOrBuildMaterial();

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> VignetteMID;

	UPROPERTY()
	TObjectPtr<APostProcessVolume> Volume;

	ECaveboundVignetteKind ActiveKind = ECaveboundVignetteKind::None;
	FLinearColor ActiveTint = FLinearColor::Red;

	float Life = 0.f;
	float Duration = 0.55f;
	float PeakStrength = 0.75f;
	float DarkenAmount = 0.f;
	bool bActive = false;
};
