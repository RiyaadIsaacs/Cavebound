#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CaveboundHoverHealth.h"
#include "CaveboundRockDeposit.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UMaterialInstanceDynamic;
class UCaveboundHarvestBar;

/**
 * A rock pile the player mines. Bigger piles take longer and then rest for a few waves.
 */
UCLASS()
class CAVEBOUND_API ACaveboundRockDeposit : public AActor, public ICaveboundHoverHealth
{
	GENERATED_BODY()

public:
	ACaveboundRockDeposit();

	virtual void BeginPlay() override;

	virtual float GetHoverHealth_Implementation() const override { return static_cast<float>(RockRemaining); }
	virtual float GetHoverMaxHealth_Implementation() const override { return static_cast<float>(RockTotal); }
	virtual FText GetHoverDisplayName_Implementation() const override { return FText::FromString(TEXT("Rock")); }

	bool IsHarvestable() const { return !bDepleted; }
	float GetMineRange() const { return MineRange; }

	// Adds harvest time. Each finished interval pays one rock. Returns true when the pile is empty.
	bool AddHarvestTime(float DeltaTime);
	void StopHarvest();

	// Called when a combat wave is cleared. Hidden piles count down toward coming back.
	void OnWaveEnded();

protected:
	void RollYield();
	void FinishHarvest();
	void WakeUp();
	void UpdateBar();
	void HideBar();
	void EnsureMineVisualMaterial();
	void UpdateMineVisuals();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rock")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rock")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rock")
	TObjectPtr<UWidgetComponent> HarvestBar;

	UPROPERTY()
	TObjectPtr<UCaveboundHarvestBar> HarvestWidget;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MineVisualMaterial;

	UPROPERTY(EditAnywhere, Category = "Rock")
	float MineRange = 300.f;

	UPROPERTY(EditAnywhere, Category = "Rock")
	float SecondsPerRock = 0.75f;

	int32 RockTotal = 12;
	int32 RockRemaining = 12;
	float TickTime = 0.f;
	int32 WavesUntilReturn = 0;
	bool bDepleted = false;
	bool bHarvesting = false;
};
