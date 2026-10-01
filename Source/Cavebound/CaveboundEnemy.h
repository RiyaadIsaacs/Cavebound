#pragma once

#include "CoreMinimal.h"
#include "CaveboundBaseEnemy.h"
#include "CaveboundEnemy.generated.h"

class USkeletalMesh;
class USkeletalMeshComponent;
class UAnimSequence;
class UAnimMontage;
class UCaveboundMinionAnimInstance;

/**
 * Path enemy using the KayKit skeleton minion.
 * Legs blend between idle, walk, and run. Hits and melee swings play on the upper body.
 */
UCLASS()
class CAVEBOUND_API ACaveboundEnemy : public ACaveboundBaseEnemy
{
	GENERATED_BODY()

public:
	ACaveboundEnemy();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void ApplyDamage(float Amount) override;

protected:
	virtual void OnDeath() override;
	virtual bool CanAttack() const override;
	virtual void OnMeleeStrike() override;

	void EnsureAnimationAssets();
	void UpdatePresentation(float DeltaTime, const FVector& LocationBeforeTick);
	bool PlayUpperBodyMontage(UAnimMontage* Montage);
	UCaveboundMinionAnimInstance* GetMinionAnim() const;

	UFUNCTION()
	void HandleHitMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> HitVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> Body;

	UPROPERTY()
	TObjectPtr<USkeletalMesh> BodyMesh;

	UPROPERTY()
	TObjectPtr<UAnimSequence> DeathAnim;

	UPROPERTY()
	TObjectPtr<UAnimMontage> HitMontage;

	UPROPERTY()
	TObjectPtr<UAnimMontage> MeleeMontage;

	// How quickly idle, walk, and run blend when speed changes. Higher is snappier.
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "0.1"))
	float LocomotionBlendSpeed = 4.f;

	float SmoothedSpeed = 0.f;
	bool bHitReacting = false;

	FTimerHandle BodyFlashTimer;
	FTimerHandle DeathTimer;
};
