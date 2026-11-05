#include "CaveboundBossModifier.h"
#include "CaveboundBossEnemy.h"

void UCaveboundBossMod_SplitOnDeath::OnBossDeath(ACaveboundBossEnemy& Boss)
{
	Boss.SpawnSplitMinions();
}

void UCaveboundBossMod_RangeBoost::Apply(ACaveboundBossEnemy& Boss)
{
	Boss.MultiplyAttackRange(1.5f);
}

void UCaveboundBossMod_EnrageTowerRush::OnBossDamaged(ACaveboundBossEnemy& Boss, float NewHealthRatio)
{
	if (NewHealthRatio < 0.5f)
	{
		Boss.TryActivateEnrage();
	}
}

void UCaveboundBossMod_PoisonAttack::OnBossDealtDamage(ACaveboundBossEnemy& Boss, AActor* Target)
{
	Boss.ApplyPoisonToTarget(Target);
}

void UCaveboundBossMod_PoisonAttack::TickBoss(ACaveboundBossEnemy& Boss, float DeltaTime)
{
	Boss.TickPoisonEffects(DeltaTime);
}

void UCaveboundBossMod_VisualMutation::Apply(ACaveboundBossEnemy& Boss)
{
	Boss.MultiplyVisualScale(1.15f);
	Boss.MultiplyAuraScale(1.2f);
}

UCaveboundBossModifier* CaveboundCreateBossModifier(UObject* Outer, ECaveboundBossModifierType Type)
{
	if (!Outer)
	{
		return nullptr;
	}

	switch (Type)
	{
	case ECaveboundBossModifierType::SplitOnDeath:
		return NewObject<UCaveboundBossMod_SplitOnDeath>(Outer);
	case ECaveboundBossModifierType::RangeBoost:
		return NewObject<UCaveboundBossMod_RangeBoost>(Outer);
	case ECaveboundBossModifierType::EnrageTowerRush:
		return NewObject<UCaveboundBossMod_EnrageTowerRush>(Outer);
	case ECaveboundBossModifierType::PoisonAttack:
		return NewObject<UCaveboundBossMod_PoisonAttack>(Outer);
	case ECaveboundBossModifierType::VisualMutation:
		return NewObject<UCaveboundBossMod_VisualMutation>(Outer);
	default:
		return nullptr;
	}
}

const TCHAR* CaveboundBossModifierTypeName(ECaveboundBossModifierType Type)
{
	switch (Type)
	{
	case ECaveboundBossModifierType::SplitOnDeath:    return TEXT("SplitOnDeath");
	case ECaveboundBossModifierType::RangeBoost:      return TEXT("RangeBoost");
	case ECaveboundBossModifierType::EnrageTowerRush: return TEXT("EnrageTowerRush");
	case ECaveboundBossModifierType::PoisonAttack:    return TEXT("PoisonAttack");
	case ECaveboundBossModifierType::VisualMutation:  return TEXT("VisualMutation");
	default:                                         return TEXT("Unknown");
	}
}
