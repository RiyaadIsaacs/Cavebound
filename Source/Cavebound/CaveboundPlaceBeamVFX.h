#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Templates/Function.h"
#include "CaveboundPlaceBeamVFX.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Vertical light beam used when placing or upgrading a defender.
 *
 * Build: yellow beam, defender fades in, then a short pop.
 * Upgrade: cyan/red beam that tints the defender (no fade-in).
 */
UCLASS()
class CAVEBOUND_API ACaveboundPlaceBeamVFX : public AActor
{
	GENERATED_BODY()

public:
	ACaveboundPlaceBeamVFX();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Yellow —used when first placing a defender. */
	static FLinearColor ColorForBuild();

	/** Cyan for 1st upgrade, red for 2nd. NextUpgradeLevel is the level after upgrade (1 or 2). */
	static FLinearColor ColorForUpgrade(int32 NextUpgradeLevel);

	/**
	 * Spawns the beam at Location.
	 * RevealActor — defender to tint / fade in (optional).
	 * bFadeIn — true = build fade; false = solid upgrade tint.
	 * OnDone — called when the beam ends and the pop starts (optional).
	 * Note: TFunction cannot be a UFUNCTION, so this is C++ only.
	 */
	static ACaveboundPlaceBeamVFX* SpawnAt(
		UWorld* World,
		const FVector& Location,
		const FLinearColor& Color,
		AActor* RevealActor = nullptr,
		bool bFadeIn = true,
		TFunction<void()> OnDone = nullptr);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	TSoftObjectPtr<UNiagaraSystem> BeamSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	TSoftObjectPtr<UNiagaraSystem> PopSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	float BeamHeight = 1300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	float BeamRadius = 42.f;

	/** How long the beam stays up before the pop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	float Duration = 1.15f;

	/** How long the pop flash lasts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	float PopDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	FLinearColor BeamColor = FLinearColor(1.f, 0.85f, 0.15f, 1.f);

	/** True = scale/fade the defender in. False = solid tint only (upgrades). */
	bool bFadeIn = true;

	TFunction<void()> OnDone;

protected:
	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<UStaticMeshComponent> BeamMesh;

	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<UStaticMeshComponent> PopMesh;

	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<UNiagaraComponent> BeamNiagara;

	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<UNiagaraComponent> PopNiagara;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BeamMat;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PopMat;

	/** Overlay tint put on the defender while the beam plays. */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> OverlayMat;

	TWeakObjectPtr<AActor> RevealActor;
	TArray<TWeakObjectPtr<UStaticMeshComponent>> Meshes;
	TArray<FVector> MeshScales;

	float Time = 0.f;
	float PopTime = 0.f;
	bool bPopping = false;
	bool bDone = false;

	void StartReveal();
	void UpdateReveal(float Progress);
	void ClearReveal();
	void StartPop();
	void Finish();
};
