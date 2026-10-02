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
	virtual FText GetHoverDisplayName_Implementation() const override { return FText::FromString(TEXT("Minion")); }

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

	// How fast the walk animation catches up when the minion speeds up or slows down.
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "0.1"))
	float LocomotionBlendSpeed = 4.f;

	// The speed we show. It eases toward the real move speed so the blend is not instant.
	float smoothedMoveSpeed = 0.f;

	// True while the hit animation is playing. The minion cannot swing during this.
	bool bIsPlayingHitAnimation = false;

	FTimerHandle BodyFlashTimer;
	FTimerHandle DeathTimer;

	void RemoveAfterDeathAnimation();
};
