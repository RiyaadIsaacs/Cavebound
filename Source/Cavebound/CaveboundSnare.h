#pragma once

#include "CoreMinimal.h"
#include "CaveboundTurret.h"
#include "CaveboundSnare.generated.h"

class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;
class UMaterialInstanceDynamic;
class ACaveboundBaseEnemy;

USTRUCT()
struct FCaveboundSnareHold
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<ACaveboundBaseEnemy> Enemy;

	UPROPERTY()
	TObjectPtr<USplineComponent> Spline;

	UPROPERTY()
	TArray<TObjectPtr<USplineMeshComponent>> Pieces;
};

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
	void ReleaseEnemy(ACaveboundBaseEnemy* Enemy);

protected:
	// Range
	UPROPERTY(EditAnywhere, Category = "Snare")
	float CatchRange = 450.f;

	// Hold duration
	UPROPERTY(EditAnywhere, Category = "Snare")
	float TrapDuration = 10.f;

	// How many enemies this snare can hold. Upgrades raise this.
	int32 GetCatchLimit() const { return 1 + UpgradeLevel; }

	virtual void ApplyLevelBonus() override;

	// Held enemies and the rope wrapped around each one.
	UPROPERTY()
	TArray<FCaveboundSnareHold> Holds;

	float PostHeight = 700.f;

	// The path the rope follows.
	UPROPERTY()
	TObjectPtr<UStaticMesh> RopeMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> RopeMaterial;

	bool IsCombatRound() const;
	void UpdateSnare();
	ACaveboundBaseEnemy* FindSnareTarget() const;
	bool IsHolding(const ACaveboundBaseEnemy* Enemy) const;
	void ShowRope(FCaveboundSnareHold& Hold);
	void ClearHold(FCaveboundSnareHold& Hold);
	void ClearAllRopes();
};
