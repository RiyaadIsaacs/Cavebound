#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CaveboundMinionAnimInstance.generated.h"

/**
 * This is the script the minion animation blueprint uses.
 * The legs use LocomotionSpeed.
 * Hits and punches play on the upper body only.
 */
UCLASS()
class CAVEBOUND_API UCaveboundMinionAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	void SetLocomotionSpeed(float Speed);
	void SetDead(bool bDead);

	// How fast the legs think the minion is moving. 0 is idle.
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float LocomotionSpeed = 0.f;

	// The animation blueprint checks this bool.
	// True plays the walk blend. False plays the death animation.
	// We store the opposite of "is dead", so it starts true and the minion walks in.
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsDead = true;

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	void EnsureUpperBodyLayer();
};
