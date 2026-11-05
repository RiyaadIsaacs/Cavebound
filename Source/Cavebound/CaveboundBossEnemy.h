#pragma once

#include "CoreMinimal.h"
#include "CaveboundEnemy.h"
#include "CaveboundBossModifier.h"
#include "CaveboundBossEnemy.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class USplineComponent;

USTRUCT()
struct FCaveboundBossPoisonVictim
{
	GENERATED_BODY()

	TWeakObjectPtr<AActor> Target;
	float TickInterval = 0.5f;
	float TimeUntilTick = 0.5f;
	int32 RemainingTicks = 6;
	float DamagePerTick = 4.f;
};

/**
 * Stacking procedural boss — KayKit Rogue mesh scaled up, modifiers applied from GameMode stack.
 */
UCLASS()
class CAVEBOUND_API ACaveboundBossEnemy : public ACaveboundEnemy
{
	GENERATED_BODY()

public:
	ACaveboundBossEnemy();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void ApplyDamage(float Amount) override;
	virtual FText GetHoverDisplayName_Implementation() const override { return FText::FromString(TEXT("Boss")); }

	/** SpawnIndex is 1-based BossSpawnCount. StackedTypes are persistent modifiers for this run. */
	void ConfigureSpawn(int32 SpawnIndex, const TArray<ECaveboundBossModifierType>& StackedTypes, bool bAsSplitMinion = false);

	void MultiplyAttackRange(float Multiplier);
	void MultiplyVisualScale(float Multiplier);
	void MultiplyAuraScale(float Multiplier);

	void TryActivateEnrage();
	bool IsEnraged() const { return bIsEnraged; }
	bool IsSplitMinion() const { return bIsSplitMinion; }

	void SpawnSplitMinions();
	void ApplyPoisonToTarget(AActor* Target);
	void TickPoisonEffects(float DeltaTime);

	USplineComponent* GetBossPathSpline() const;
	ACaveboundTree* GetBossTree() const { return Tree.Get(); }
	float GetPreferredLane() const { return PreferredLaneOffset; }

protected:
	virtual void OnDeath() override;
	virtual AActor* ResolveAttackTarget() const override;
	virtual void AttackCurrentTarget(float DeltaTime) override;
	virtual void EnsureAnimationAssets() override;

	void ApplyModifiers();
	void ApplyStatScaling(int32 SpawnIndex);
	void SetupAuraVisuals(int32 SpawnIndex);
	void TriggerEnrageVisuals();
	void WireBossCombatMontages();
	FLinearColor ResolveTierAuraColor(int32 SpawnIndex) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss")
	TObjectPtr<UStaticMeshComponent> AuraSphere;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> AuraMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> EnrageMID;

	UPROPERTY()
	TArray<TObjectPtr<UCaveboundBossModifier>> ActiveModifiers;

	UPROPERTY()
	TArray<ECaveboundBossModifierType> StackedModifierTypes;

	UPROPERTY(EditAnywhere, Category = "Boss")
	float BossMeshScale = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Boss")
	float AuraBaseScale = 3.5f;

	UPROPERTY(EditAnywhere, Category = "Boss|Poison")
	float PoisonTickInterval = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Boss|Poison")
	int32 PoisonTickCount = 6;

	UPROPERTY(EditAnywhere, Category = "Boss|Poison")
	float PoisonDamageFraction = 0.25f;

	int32 BossSpawnIndex = 1;
	float AuraScaleMultiplier = 1.f;
	float PreEnrageMoveSpeed = 0.f;
	bool bIsEnraged = false;
	bool bIsSplitMinion = false;
	bool bConfigured = false;

	TArray<FCaveboundBossPoisonVictim> PoisonVictims;
};
