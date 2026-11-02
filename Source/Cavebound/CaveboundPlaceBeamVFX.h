#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CaveboundPlaceBeamVFX.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Vertical beam for place/upgrade. Fades the reveal actor in with the beam color,
 * then plays a pop burst when the beam ends and the defender finishes spawning in.
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

	static FLinearColor ColorForBuild();
	static FLinearColor ColorForUpgrade(int32 NextUpgradeLevel);

	// C++ only — TFunction is not UHT-safe.
	static ACaveboundPlaceBeamVFX* SpawnAt(
		UWorld* World,
		const FVector& Location,
		const FLinearColor& Color,
		AActor* RevealActor = nullptr,
		TFunction<void()> Finished = nullptr);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	TSoftObjectPtr<UNiagaraSystem> BeamSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	TSoftObjectPtr<UNiagaraSystem> PopSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	float BeamHeight = 1300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	float BeamRadius = 42.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	float Duration = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	float PopDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	FLinearColor BeamColor = FLinearColor(1.f, 0.85f, 0.15f, 1.f);

	TFunction<void()> OnFinished;

protected:
	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<UStaticMeshComponent> BeamMesh;

	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<UStaticMeshComponent> PopMesh;

	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<UNiagaraComponent> Niagara;

	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<UNiagaraComponent> PopNiagara;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BeamMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> PopMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> RevealMID;

	TWeakObjectPtr<AActor> RevealActor;
	TArray<TWeakObjectPtr<UStaticMeshComponent>> RevealMeshes;
	TArray<FVector> RevealBaseScales;

	float Age = 0.f;
	float PopAge = 0.f;
	bool bInPop = false;
	bool bFinished = false;

	void BeginReveal();
	void UpdateReveal(float Alpha);
	void ClearReveal();
	void StartPop();
	void FinishAndDestroy();
};
