#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CaveboundMoveMarker.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/** Pulsing ground ring that marks the click-to-move destination. */
UCLASS()
class CAVEBOUND_API ACaveboundMoveMarker : public AActor
{
	GENERATED_BODY()

public:
	ACaveboundMoveMarker();

	virtual void Tick(float DeltaTime) override;

	void ShowAt(const FVector& WorldLocation);
	void Hide();

	UPROPERTY(EditAnywhere, Category = "VFX")
	FLinearColor Color = FLinearColor(1.f, 0.92f, 0.35f, 1.f);

	UPROPERTY(EditAnywhere, Category = "VFX")
	float Radius = 70.f;

	UPROPERTY(EditAnywhere, Category = "VFX")
	float PulseSpeed = 3.2f;

	/** Lift above the ground so the ring doesn't z-fight. */
	UPROPERTY(EditAnywhere, Category = "VFX")
	float GroundOffset = 4.f;

protected:
	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<UStaticMeshComponent> Outer;

	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<UStaticMeshComponent> Inner;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> OuterMat;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> InnerMat;

	float PulseTime = 0.f;
	bool bVisible = false;

	void UpdatePulse(float Amount);
};
