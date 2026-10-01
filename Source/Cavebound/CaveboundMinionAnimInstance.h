#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CaveboundMinionAnimInstance.generated.h"

/**
 * Drives the minion animation blueprint. Legs follow LocomotionSpeed.
 * Hit and melee montages play on the upper-body slot.
 */
UCLASS()
class CAVEBOUND_API UCaveboundMinionAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	void SetLocomotionSpeed(float Speed);
	void SetDead(bool bDead);

	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float LocomotionSpeed = 0.f;

	// The bool blend plays its first pose when this is true, and that pose is locomotion.
	// Death is the second pose, so this stays true until the minion dies.
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsDead = true;

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	void EnsureUpperBodyLayer();
};
