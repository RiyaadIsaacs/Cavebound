#pragma once

#include "CoreMinimal.h"
#include "CaveboundTurret.h"
#include "CaveboundSnare.generated.h"

class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;
class UMaterialInstanceDynamic;
class ACaveboundBaseEnemy;

//Grabs enemies and prevents them from acting
UCLASS(Blueprintable)
class CAVEBOUND_API ACaveboundSnare : public ACaveboundTurret
{
	GENERATED_BODY()

public:
	ACaveboundSnare();

	virtual FText GetHoverDisplayName_Implementation() const override { return FText::FromString(TEXT("Snare")); }

	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void DropCatch();

protected:
	// Range
	UPROPERTY(EditAnywhere, Category = "Snare")
	float CatchRange = 450.f;

	// Hold duration
	UPROPERTY(EditAnywhere, Category = "Snare")
	float TrapDuration = 10.f;

	// Held enemy
	TWeakObjectPtr<ACaveboundBaseEnemy> TrappedEnemy;

	float PostHeight = 700.f;

	// The path the rope follows.
	UPROPERTY()
	TObjectPtr<USplineComponent> RopeSpline;

	UPROPERTY()
	TArray<TObjectPtr<USplineMeshComponent>> RopePieces;

	UPROPERTY()
	TObjectPtr<UStaticMesh> RopeMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> RopeMaterial;

	bool IsCombatRound() const;
	void UpdateSnare();
	ACaveboundBaseEnemy* FindSnareTarget() const;
	void ShowRope(ACaveboundBaseEnemy* CaughtEnemy);
	void ClearRope();
};
