#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CaveboundBossModifier.generated.h"

class ACaveboundBossEnemy;

UENUM(BlueprintType)
enum class ECaveboundBossModifierType : uint8
{
	SplitOnDeath      UMETA(DisplayName = "Split On Death"),
	RangeBoost        UMETA(DisplayName = "Increased Range"),
	EnrageTowerRush   UMETA(DisplayName = "Enraged Charge"),
	PoisonAttack      UMETA(DisplayName = "Poison Damage"),
	VisualMutation    UMETA(DisplayName = "Visual Mutation"),
};

/** Modular boss behaviour applied when a stacked modifier is active. */
UCLASS(Abstract)
class CAVEBOUND_API UCaveboundBossModifier : public UObject
{
	GENERATED_BODY()

public:
	virtual ECaveboundBossModifierType GetType() const PURE_VIRTUAL(UCaveboundBossModifier::GetType, return ECaveboundBossModifierType::SplitOnDeath;);
	virtual FString GetDebugName() const PURE_VIRTUAL(UCaveboundBossModifier::GetDebugName, return TEXT("?"););

	virtual void Apply(ACaveboundBossEnemy& Boss) {}
	virtual void TickBoss(ACaveboundBossEnemy& Boss, float DeltaTime) {}
	virtual void OnBossDamaged(ACaveboundBossEnemy& Boss, float NewHealthRatio) {}
	virtual void OnBossDealtDamage(ACaveboundBossEnemy& Boss, AActor* Target) {}
	virtual void OnBossDeath(ACaveboundBossEnemy& Boss) {}
};

UCLASS()
class CAVEBOUND_API UCaveboundBossMod_SplitOnDeath : public UCaveboundBossModifier
{
	GENERATED_BODY()
public:
	virtual ECaveboundBossModifierType GetType() const override { return ECaveboundBossModifierType::SplitOnDeath; }
	virtual FString GetDebugName() const override { return TEXT("SplitOnDeath"); }
	virtual void OnBossDeath(ACaveboundBossEnemy& Boss) override;
};

UCLASS()
class CAVEBOUND_API UCaveboundBossMod_RangeBoost : public UCaveboundBossModifier
{
	GENERATED_BODY()
public:
	virtual ECaveboundBossModifierType GetType() const override { return ECaveboundBossModifierType::RangeBoost; }
	virtual FString GetDebugName() const override { return TEXT("RangeBoost"); }
	virtual void Apply(ACaveboundBossEnemy& Boss) override;
};

UCLASS()
class CAVEBOUND_API UCaveboundBossMod_EnrageTowerRush : public UCaveboundBossModifier
{
	GENERATED_BODY()
public:
	virtual ECaveboundBossModifierType GetType() const override { return ECaveboundBossModifierType::EnrageTowerRush; }
	virtual FString GetDebugName() const override { return TEXT("EnrageTowerRush"); }
	virtual void OnBossDamaged(ACaveboundBossEnemy& Boss, float NewHealthRatio) override;
};

UCLASS()
class CAVEBOUND_API UCaveboundBossMod_PoisonAttack : public UCaveboundBossModifier
{
	GENERATED_BODY()
public:
	virtual ECaveboundBossModifierType GetType() const override { return ECaveboundBossModifierType::PoisonAttack; }
	virtual FString GetDebugName() const override { return TEXT("PoisonAttack"); }
	virtual void OnBossDealtDamage(ACaveboundBossEnemy& Boss, AActor* Target) override;
	virtual void TickBoss(ACaveboundBossEnemy& Boss, float DeltaTime) override;
};

UCLASS()
class CAVEBOUND_API UCaveboundBossMod_VisualMutation : public UCaveboundBossModifier
{
	GENERATED_BODY()
public:
	virtual ECaveboundBossModifierType GetType() const override { return ECaveboundBossModifierType::VisualMutation; }
	virtual FString GetDebugName() const override { return TEXT("VisualMutation"); }
	virtual void Apply(ACaveboundBossEnemy& Boss) override;
};

UCaveboundBossModifier* CaveboundCreateBossModifier(UObject* Outer, ECaveboundBossModifierType Type);
const TCHAR* CaveboundBossModifierTypeName(ECaveboundBossModifierType Type);
