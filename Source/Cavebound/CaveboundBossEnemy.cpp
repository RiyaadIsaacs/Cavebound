#include "CaveboundBossEnemy.h"

#include "CaveboundGameMode.h"
#include "CaveboundMinionAnimInstance.h"
#include "CaveboundShaderMaterials.h"
#include "CaveboundTree.h"
#include "CaveboundTurret.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace CaveboundBossPaths
{
	const TCHAR* Mesh = TEXT("/Game/Characters/Skeleton/Skeleton_Rogue.Skeleton_Rogue");
	const TCHAR* Melee = TEXT("/Game/Characters/Skeleton/Rig_Medium_CombatMeleeMelee_Dualwield_Attack_Slice.Rig_Medium_CombatMeleeMelee_Dualwield_Attack_Slice");
	const TCHAR* MeleeAlt = TEXT("/Game/Characters/Skeleton/Rig_Medium_CombatMeleeMelee_Dualwield_Attack_Chop.Rig_Medium_CombatMeleeMelee_Dualwield_Attack_Chop");
	const TCHAR* Hit = TEXT("/Game/Characters/Skeleton/Rig_Medium_GeneralHit_B.Rig_Medium_GeneralHit_B");
	const TCHAR* Death = TEXT("/Game/Characters/Skeleton/Rig_Medium_GeneralDeath_B.Rig_Medium_GeneralDeath_B");
}

ACaveboundBossEnemy::ACaveboundBossEnemy()
{
	VisualMesh->SetRelativeScale3D(FVector(1.f));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> RogueMesh(CaveboundBossPaths::Mesh);
	if (RogueMesh.Succeeded())
	{
		BodyMesh = RogueMesh.Object;
		Body->SetSkeletalMeshAsset(BodyMesh);
	}

	AuraSphere = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AuraSphere"));
	AuraSphere->SetupAttachment(VisualMesh);
	AuraSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AuraSphere->SetCastShadow(false);
	AuraSphere->bDisallowNanite = true;
	AuraSphere->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	AuraSphere->SetRelativeScale3D(FVector(AuraBaseScale));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		AuraSphere->SetStaticMesh(SphereMesh.Object);
	}

	if (HitVolume)
	{
		HitVolume->SetRelativeScale3D(FVector(1.8f, 1.8f, 3.0f));
		HitVolume->SetRelativeLocation(FVector(0.f, 0.f, 140.f));
	}

	MinDifficultyToSpawn = 9999; // Never picked by Adaptive Threat Budget pool
	MaxHealth = 480.f;
	Health = MaxHealth;
	MoveSpeed = 160.f;
	AttackDamage = 22.f;
	AttackRange = 420.f;
	AttackInterval = 1.2f;
	TurretDetectRange = 1200.f;
	PathHeightOffset = 0.f;
	HitFlashColor = FLinearColor(0.7f, 0.15f, 0.85f);
	StandClearDistance = 200.f;
}

void ACaveboundBossEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (Body)
	{
		Body->SetRelativeScale3D(FVector(BossMeshScale));
	}
}

void ACaveboundBossEnemy::EnsureAnimationAssets()
{
	Super::EnsureAnimationAssets();

	if (!BodyMesh)
	{
		BodyMesh = LoadObject<USkeletalMesh>(nullptr, CaveboundBossPaths::Mesh);
	}
	if (Body && BodyMesh)
	{
		Body->SetSkeletalMeshAsset(BodyMesh);
		Body->SetRelativeScale3D(FVector(BossMeshScale));
	}

	if (UAnimSequence* BossDeath = LoadObject<UAnimSequence>(nullptr, CaveboundBossPaths::Death))
	{
		DeathAnim = BossDeath;
	}

	WireBossCombatMontages();

	if (UCaveboundMinionAnimInstance* Anim = GetMinionAnim())
	{
		Anim->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	}
}

void ACaveboundBossEnemy::WireBossCombatMontages()
{
	auto MakeUpperBodyMontage = [](UAnimSequence* Sequence) -> UAnimMontage*
	{
		if (!Sequence)
		{
			return nullptr;
		}
		return UAnimMontage::CreateSlotAnimationAsDynamicMontage(
			Sequence,
			FName(TEXT("UpperBody")),
			0.12f,
			0.18f,
			1.f,
			1);
	};

	UAnimSequence* MeleeSeq = LoadObject<UAnimSequence>(nullptr, CaveboundBossPaths::Melee);
	if (!MeleeSeq)
	{
		MeleeSeq = LoadObject<UAnimSequence>(nullptr, CaveboundBossPaths::MeleeAlt);
	}
	if (UAnimMontage* BossMelee = MakeUpperBodyMontage(MeleeSeq))
	{
		MeleeMontage = BossMelee;
	}

	if (UAnimSequence* HitSeq = LoadObject<UAnimSequence>(nullptr, CaveboundBossPaths::Hit))
	{
		if (UAnimMontage* BossHit = MakeUpperBodyMontage(HitSeq))
		{
			HitMontage = BossHit;
		}
	}
}

void ACaveboundBossEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsDead())
	{
		return;
	}

	for (UCaveboundBossModifier* Mod : ActiveModifiers)
	{
		if (Mod)
		{
			Mod->TickBoss(*this, DeltaTime);
		}
	}
}

void ACaveboundBossEnemy::ConfigureSpawn(
	int32 SpawnIndex,
	const TArray<ECaveboundBossModifierType>& StackedTypes,
	bool bAsSplitMinion)
{
	bConfigured = true;
	bIsSplitMinion = bAsSplitMinion;
	BossSpawnIndex = FMath::Max(1, SpawnIndex);
	StackedModifierTypes = StackedTypes;

	if (!bIsSplitMinion)
	{
		ApplyStatScaling(BossSpawnIndex);
	}

	ApplyModifiers();
	SetupAuraVisuals(BossSpawnIndex);

	FString ModNames;
	for (ECaveboundBossModifierType Type : StackedModifierTypes)
	{
		if (!ModNames.IsEmpty())
		{
			ModNames += TEXT(", ");
		}
		ModNames += CaveboundBossModifierTypeName(Type);
	}
	if (ModNames.IsEmpty())
	{
		ModNames = TEXT("(none)");
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Boss spawn #%d%s HP=%.0f Dmg=%.1f Range=%.0f Scale=%.2f Mods=[%s]"),
		BossSpawnIndex,
		bIsSplitMinion ? TEXT(" (split minion)") : TEXT(""),
		MaxHealth,
		AttackDamage,
		AttackRange,
		BossMeshScale,
		*ModNames);
}

void ACaveboundBossEnemy::ApplyStatScaling(int32 SpawnIndex)
{
	const int32 N = FMath::Max(1, SpawnIndex);
	const float HealthMul = FMath::Pow(1.40f, static_cast<float>(N - 1));
	const float DamageMul = FMath::Pow(1.25f, static_cast<float>(N - 1));
	const float RangeMul = FMath::Pow(1.15f, static_cast<float>(N - 1));

	MaxHealth *= HealthMul;
	Health = MaxHealth;
	AttackDamage *= DamageMul;
	AttackRange *= RangeMul;
	TurretDetectRange *= RangeMul;
}

void ACaveboundBossEnemy::ApplyModifiers()
{
	ActiveModifiers.Reset();
	for (ECaveboundBossModifierType Type : StackedModifierTypes)
	{
		if (UCaveboundBossModifier* Mod = CaveboundCreateBossModifier(this, Type))
		{
			ActiveModifiers.Add(Mod);
			Mod->Apply(*this);
		}
	}

	if (Body)
	{
		Body->SetRelativeScale3D(FVector(BossMeshScale));
	}
	if (AuraSphere)
	{
		AuraSphere->SetRelativeScale3D(FVector(AuraBaseScale * AuraScaleMultiplier));
	}
}

void ACaveboundBossEnemy::MultiplyAttackRange(float Multiplier)
{
	AttackRange *= Multiplier;
	TurretDetectRange *= Multiplier;
}

void ACaveboundBossEnemy::MultiplyVisualScale(float Multiplier)
{
	BossMeshScale *= Multiplier;
	if (Body)
	{
		Body->SetRelativeScale3D(FVector(BossMeshScale));
	}
}

void ACaveboundBossEnemy::MultiplyAuraScale(float Multiplier)
{
	AuraScaleMultiplier *= Multiplier;
	if (AuraSphere)
	{
		AuraSphere->SetRelativeScale3D(FVector(AuraBaseScale * AuraScaleMultiplier));
	}
}

FLinearColor ACaveboundBossEnemy::ResolveTierAuraColor(int32 SpawnIndex) const
{
	if (SpawnIndex >= 7)
	{
		return FLinearColor(1.f, 0.f, 0.f, 1.f); // red
	}
	if (SpawnIndex >= 5)
	{
		return FLinearColor(1.f, 0.4f, 0.f, 1.f); // #FF6600
	}
	if (SpawnIndex >= 3)
	{
		return FLinearColor(0.f, 0.533f, 1.f, 1.f); // #0088FF
	}
	return FLinearColor(1.f, 1.f, 1.f, 1.f); // white
}

void ACaveboundBossEnemy::SetupAuraVisuals(int32 SpawnIndex)
{
	if (!AuraSphere)
	{
		return;
	}

	AuraMID = CaveboundShaderMaterials::CreateTierAuraMID(this);
	if (!AuraMID)
	{
		AuraMID = CaveboundShaderMaterials::CreatePoisonPulseMID(this);
	}

	if (AuraMID)
	{
		const FLinearColor Color = ResolveTierAuraColor(SpawnIndex);
		AuraMID->SetVectorParameterValue(TEXT("AuraColor"), Color);
		AuraMID->SetVectorParameterValue(TEXT("AuraColorA"), Color);
		AuraMID->SetVectorParameterValue(TEXT("AuraColorB"), Color * 1.25f);
		AuraMID->SetVectorParameterValue(TEXT("GreenA"), Color);
		AuraMID->SetVectorParameterValue(TEXT("GreenB"), Color * 1.25f);
		AuraMID->SetScalarParameterValue(TEXT("Opacity"), 0.28f);
		AuraMID->SetScalarParameterValue(TEXT("PulseSpeed"), 1.2f);
		AuraSphere->SetMaterial(0, AuraMID);
	}

	AuraSphere->SetVisibility(true);
	AuraSphere->SetHiddenInGame(false);
	AuraSphere->SetRelativeScale3D(FVector(AuraBaseScale * AuraScaleMultiplier));
}

void ACaveboundBossEnemy::ApplyDamage(float Amount)
{
	const bool bWasAlive = !IsDead();
	Super::ApplyDamage(Amount);

	if (!bWasAlive || MaxHealth <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float Ratio = Health / MaxHealth;
	for (UCaveboundBossModifier* Mod : ActiveModifiers)
	{
		if (Mod)
		{
			Mod->OnBossDamaged(*this, Ratio);
		}
	}
}

void ACaveboundBossEnemy::TryActivateEnrage()
{
	if (bIsEnraged || IsDead())
	{
		return;
	}

	bIsEnraged = true;
	PreEnrageMoveSpeed = MoveSpeed;
	MoveSpeed *= 2.f;
	TriggerEnrageVisuals();

	UE_LOG(LogTemp, Log, TEXT("Boss enraged: MoveSpeed %.0f -> %.0f, tree-only rush"), PreEnrageMoveSpeed, MoveSpeed);
}

void ACaveboundBossEnemy::TriggerEnrageVisuals()
{
	EnrageMID = CaveboundShaderMaterials::CreateEnragePulseMID(this);
	if (EnrageMID && Body)
	{
		EnrageMID->SetVectorParameterValue(TEXT("_EnrageGlowColor"), FLinearColor(0.65f, 0.1f, 1.f, 1.f));
		EnrageMID->SetVectorParameterValue(TEXT("PurpleA"), FLinearColor(0.35f, 0.05f, 0.55f, 1.f));
		EnrageMID->SetVectorParameterValue(TEXT("PurpleB"), FLinearColor(0.85f, 0.2f, 1.f, 1.f));
		EnrageMID->SetScalarParameterValue(TEXT("DisplacementStrength"), 4.f);
		EnrageMID->SetScalarParameterValue(TEXT("PulseSpeed"), 2.5f);

		const int32 NumMats = Body->GetNumMaterials();
		for (int32 i = 0; i < NumMats; ++i)
		{
			Body->SetMaterial(i, EnrageMID);
		}
	}

	if (AuraMID)
	{
		const FLinearColor Purple(0.7f, 0.15f, 1.f, 1.f);
		AuraMID->SetVectorParameterValue(TEXT("AuraColor"), Purple);
		AuraMID->SetVectorParameterValue(TEXT("AuraColorA"), Purple);
		AuraMID->SetVectorParameterValue(TEXT("AuraColorB"), Purple * 1.3f);
		AuraMID->SetVectorParameterValue(TEXT("GreenA"), Purple);
		AuraMID->SetVectorParameterValue(TEXT("GreenB"), Purple * 1.3f);
		AuraMID->SetScalarParameterValue(TEXT("Opacity"), 0.4f);
		AuraMID->SetScalarParameterValue(TEXT("PulseSpeed"), 2.8f);
	}
}

AActor* ACaveboundBossEnemy::ResolveAttackTarget() const
{
	if (bIsEnraged)
	{
		ACaveboundTree* CurrentTree = Tree.Get();
		if (CurrentTree && !CurrentTree->IsDestroyed())
		{
			return CurrentTree;
		}
		return nullptr;
	}

	return Super::ResolveAttackTarget();
}

void ACaveboundBossEnemy::AttackCurrentTarget(float DeltaTime)
{
	AActor* Target = ResolveAttackTarget();
	if (!IsValid(Target))
	{
		return;
	}

	FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	if (!ToTarget.IsNearlyZero())
	{
		SetActorRotation(ToTarget.Rotation());
	}

	if (!CanAttack())
	{
		return;
	}

	AttackTime += DeltaTime;
	if (AttackTime < AttackInterval)
	{
		return;
	}

	AttackTime = 0.f;
	OnMeleeStrike();
	if (!AttackAppliesDamageNow())
	{
		return;
	}

	bool bDealt = false;
	if (ACaveboundTurret* Turret = Cast<ACaveboundTurret>(Target))
	{
		Turret->ApplyDamage(AttackDamage);
		bDealt = true;
	}
	else if (ACaveboundTree* TreeTarget = Cast<ACaveboundTree>(Target))
	{
		TreeTarget->ApplyDamage(AttackDamage);
		bDealt = true;
	}

	if (bDealt)
	{
		for (UCaveboundBossModifier* Mod : ActiveModifiers)
		{
			if (Mod)
			{
				Mod->OnBossDealtDamage(*this, Target);
			}
		}
	}
}

void ACaveboundBossEnemy::ApplyPoisonToTarget(AActor* Target)
{
	if (!IsValid(Target))
	{
		return;
	}

	for (FCaveboundBossPoisonVictim& Existing : PoisonVictims)
	{
		if (Existing.Target.Get() == Target)
		{
			Existing.RemainingTicks = PoisonTickCount;
			Existing.TimeUntilTick = PoisonTickInterval;
			Existing.DamagePerTick = AttackDamage * PoisonDamageFraction;
			return;
		}
	}

	FCaveboundBossPoisonVictim Victim;
	Victim.Target = Target;
	Victim.TickInterval = PoisonTickInterval;
	Victim.TimeUntilTick = PoisonTickInterval;
	Victim.RemainingTicks = PoisonTickCount;
	Victim.DamagePerTick = AttackDamage * PoisonDamageFraction;
	PoisonVictims.Add(Victim);
}

void ACaveboundBossEnemy::TickPoisonEffects(float DeltaTime)
{
	for (int32 i = PoisonVictims.Num() - 1; i >= 0; --i)
	{
		FCaveboundBossPoisonVictim& Victim = PoisonVictims[i];
		AActor* Target = Victim.Target.Get();
		if (!IsValid(Target) || Victim.RemainingTicks <= 0)
		{
			PoisonVictims.RemoveAt(i);
			continue;
		}

		Victim.TimeUntilTick -= DeltaTime;
		if (Victim.TimeUntilTick > 0.f)
		{
			continue;
		}

		Victim.TimeUntilTick = Victim.TickInterval;
		--Victim.RemainingTicks;

		if (ACaveboundTurret* Turret = Cast<ACaveboundTurret>(Target))
		{
			Turret->ApplyDamage(Victim.DamagePerTick);
		}
		else if (ACaveboundTree* TreeTarget = Cast<ACaveboundTree>(Target))
		{
			TreeTarget->ApplyDamage(Victim.DamagePerTick);
		}
	}
}

void ACaveboundBossEnemy::SpawnSplitMinions()
{
	if (bIsSplitMinion)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>();
	USplineComponent* Spline = GetBossPathSpline();
	ACaveboundTree* TreeTarget = GetBossTree();

	const float MinionHealth = MaxHealth * 0.4f;
	const FVector BaseLoc = GetActorLocation();

	for (int32 i = 0; i < 2; ++i)
	{
		const FVector Offset = FVector((i == 0) ? -80.f : 80.f, 0.f, 0.f);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.Owner = GameMode;

		ACaveboundBossEnemy* Minion = World->SpawnActor<ACaveboundBossEnemy>(
			ACaveboundBossEnemy::StaticClass(),
			BaseLoc + Offset,
			GetActorRotation(),
			Params);
		if (!Minion)
		{
			continue;
		}

		Minion->BossMeshScale = BossMeshScale * 0.5f;
		Minion->ConfigureSpawn(BossSpawnIndex, StackedModifierTypes, true);
		Minion->MaxHealth = MinionHealth;
		Minion->Health = MinionHealth;
		Minion->AttackDamage = AttackDamage * 0.5f;
		Minion->AttackRange = AttackRange * 0.85f;
		Minion->TurretDetectRange = TurretDetectRange * 0.85f;
		Minion->MoveSpeed = MoveSpeed * 1.1f;
		Minion->InitAlongPath(Spline, TreeTarget, PreferredLaneOffset + ((i == 0) ? -40.f : 40.f));

		if (GameMode)
		{
			GameMode->RegisterExtraEnemyAlive();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Boss SplitOnDeath: spawned 2 minions (HP=%.0f each)"), MinionHealth);
}

USplineComponent* ACaveboundBossEnemy::GetBossPathSpline() const
{
	return PathSpline.Get();
}

void ACaveboundBossEnemy::OnDeath()
{
	for (UCaveboundBossModifier* Mod : ActiveModifiers)
	{
		if (Mod)
		{
			Mod->OnBossDeath(*this);
		}
	}

	if (AuraSphere)
	{
		AuraSphere->SetVisibility(false);
	}

	Super::OnDeath();
}
