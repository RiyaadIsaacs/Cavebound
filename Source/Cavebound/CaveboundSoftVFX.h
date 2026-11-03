#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CaveboundSoftVFX.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/** Short burst of colored sparks — used when an enemy takes a hit. */
UCLASS()
class CAVEBOUND_API ACaveboundSparkBurst : public AActor
{
	GENERATED_BODY()

public:
	ACaveboundSparkBurst();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	static void SpawnAt(UWorld* World, const FVector& Location, const FLinearColor& Color);

	UPROPERTY(EditAnywhere, Category = "VFX")
	FLinearColor Color = FLinearColor(1.f, 0.85f, 0.15f, 1.f);

	UPROPERTY(EditAnywhere, Category = "VFX")
	int32 Count = 14;

	UPROPERTY(EditAnywhere, Category = "VFX")
	float Duration = 0.35f;

protected:
	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Balls;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Mats;

	TArray<FVector> Speeds;
	float Time = 0.f;
};

/** Soft glowing orbs that drift around a parent (e.g. green embers on the tree). */
UCLASS()
class CAVEBOUND_API ACaveboundHoverEmbers : public AActor
{
	GENERATED_BODY()

public:
	ACaveboundHoverEmbers();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	static ACaveboundHoverEmbers* SpawnAttached(
		AActor* Parent,
		const FVector& LocalOffset,
		const FLinearColor& Color);

	UPROPERTY(EditAnywhere, Category = "VFX")
	FLinearColor Color = FLinearColor(0.18f, 0.42f, 0.12f, 1.f);

	UPROPERTY(EditAnywhere, Category = "VFX")
	int32 Count = 48;

	UPROPERTY(EditAnywhere, Category = "VFX")
	float Radius = 220.f;

	UPROPERTY(EditAnywhere, Category = "VFX")
	float Height = 140.f;

protected:
	UPROPERTY(VisibleAnywhere, Category = "VFX")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Balls;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Mats;

	TArray<float> Phase;
	TArray<float> RadiusScale;
	float Time = 0.f;
};
