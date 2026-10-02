#include "CaveboundEnemy.h"
#include "CaveboundDamageFlash.h"
#include "CaveboundGameMode.h"
#include "CaveboundMinionAnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

// The assets the minion uses. Mesh, walk blueprint, death clip, hit clip, and punch clip.
namespace CaveboundMinionPaths
{
	const TCHAR* Mesh = TEXT("/Game/Characters/Skeleton/Skeleton_Minion.Skeleton_Minion");
	const TCHAR* AnimClass = TEXT("/Game/Characters/Skeleton/ABP_Minion.ABP_Minion_C");
	const TCHAR* Death = TEXT("/Game/Characters/Skeleton/Rig_Medium_GeneralDeath_A.Rig_Medium_GeneralDeath_A");
	const TCHAR* Hit = TEXT("/Game/Characters/Skeleton/AM_MinionHit.AM_MinionHit");
	const TCHAR* Melee = TEXT("/Game/Characters/Skeleton/AM_MinionMelee.AM_MinionMelee");
}

ACaveboundEnemy::ACaveboundEnemy()
{
	// Feet sit on the path, so we do not lift the actor extra.
	PathHeightOffset = 0.f;

	// Keep VisualMesh visible in the hierarchy. SetVisibility(false) on the root would
	// hide Body via IsVisible()'s parent walk, even with bPropagateToChildren=false.
	// Clear any placeholder draw and leave hit detection to HitVolume.
	VisualMesh->SetStaticMesh(nullptr);
	VisualMesh->SetVisibility(true);
	VisualMesh->SetHiddenInGame(false);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	// Overlap volume so arrows (WorldDynamic) can hit the minion.
	// Engine cube is 100cm; scale ~1.2 x 1.2 x 2.0 covers a standing character.
	HitVolume = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HitVolume"));
	HitVolume->SetupAttachment(VisualMesh);
	HitVolume->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	HitVolume->SetRelativeScale3D(FVector(1.2f, 1.2f, 2.0f));
	HitVolume->SetVisibility(false);
	HitVolume->SetHiddenInGame(true);
	HitVolume->SetCastShadow(false);
	HitVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HitVolume->SetCollisionObjectType(ECC_WorldDynamic);
	HitVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	HitVolume->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	HitVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	HitVolume->SetGenerateOverlapEvents(true);
	if (CubeMesh.Succeeded())
	{
		HitVolume->SetStaticMesh(CubeMesh.Object);
	}

	// Skeletal body stays on the visible VisualMesh root so it actually draws.
	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(VisualMesh);
	Body->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetVisibility(true);
	Body->SetHiddenInGame(false);
	Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// Load the minion mesh, walk blueprint, death clip, hit clip, and punch clip.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(CaveboundMinionPaths::Mesh);
	if (MeshAsset.Succeeded())
	{
		BodyMesh = MeshAsset.Object;
		Body->SetSkeletalMeshAsset(BodyMesh);
	}
	else if (CubeMesh.Succeeded())
	{
		// Fallback so a missing skeletal asset still shows something playable.
		VisualMesh->SetStaticMesh(CubeMesh.Object);
		VisualMesh->SetRelativeScale3D(FVector(0.8f, 0.5f, 2.0f));
		VisualMesh->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
		VisualMesh->SetVisibility(true);
		VisualMesh->SetHiddenInGame(false);
		VisualMesh->SetCastShadow(true);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimClass(CaveboundMinionPaths::AnimClass);
	if (AnimClass.Succeeded())
	{
		Body->SetAnimInstanceClass(AnimClass.Class);
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathAsset(CaveboundMinionPaths::Death);
	if (DeathAsset.Succeeded())
	{
		DeathAnim = DeathAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> HitAsset(CaveboundMinionPaths::Hit);
	if (HitAsset.Succeeded())
	{
		HitMontage = HitAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> MeleeAsset(CaveboundMinionPaths::Melee);
	if (MeleeAsset.Succeeded())
	{
		MeleeMontage = MeleeAsset.Object;
	}

	// Defaults for this enemy type (Adaptive Threat Budget gate)
	MinDifficultyToSpawn = 0;
	MaxHealth = 60.f;
	Health = MaxHealth;
	MoveSpeed = 280.f;
	AttackDamage = 8.f;
	AttackRange = 280.f;
	AttackInterval = 1.0f;
}

void ACaveboundEnemy::EnsureAnimationAssets()
{
	if (!Body)
	{
		return;
	}

	// Parent VisualMesh stays visible so Body is not culled by the parent IsVisible walk.
	if (VisualMesh)
	{
		VisualMesh->SetVisibility(true);
		VisualMesh->SetHiddenInGame(false);
	}
	Body->SetVisibility(true);
	Body->SetHiddenInGame(false);

	// If the constructor did not find an asset, try loading it again when the game starts.

	if (!BodyMesh)
	{
		BodyMesh = LoadObject<USkeletalMesh>(nullptr, CaveboundMinionPaths::Mesh);
	}
	if (BodyMesh)
	{
		Body->SetSkeletalMeshAsset(BodyMesh);
		// Drop any fallback cube once the real mesh is available.
		if (VisualMesh && VisualMesh->GetStaticMesh())
		{
			VisualMesh->SetStaticMesh(nullptr);
			VisualMesh->SetRelativeScale3D(FVector::OneVector);
			VisualMesh->SetRelativeLocation(FVector::ZeroVector);
			VisualMesh->SetCastShadow(false);
		}
	}
	else if (VisualMesh && !VisualMesh->GetStaticMesh())
	{
		if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
		{
			VisualMesh->SetStaticMesh(Cube);
			VisualMesh->SetRelativeScale3D(FVector(0.8f, 0.5f, 2.0f));
			VisualMesh->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
			VisualMesh->SetVisibility(true);
			VisualMesh->SetHiddenInGame(false);
			VisualMesh->SetCastShadow(true);
		}
	}

	if (HitVolume)
	{
		HitVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		HitVolume->SetCollisionObjectType(ECC_WorldDynamic);
		HitVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
		HitVolume->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
		HitVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		HitVolume->SetGenerateOverlapEvents(true);
		if (!HitVolume->GetStaticMesh())
		{
			if (UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
			{
				HitVolume->SetStaticMesh(Cube);
			}
		}
	}

	if (!Body->GetAnimClass())
	{
		if (UClass* AnimClass = LoadClass<UAnimInstance>(nullptr, CaveboundMinionPaths::AnimClass))
		{
			Body->SetAnimInstanceClass(AnimClass);
		}
	}

	Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	if (!DeathAnim)
	{
		DeathAnim = LoadObject<UAnimSequence>(nullptr, CaveboundMinionPaths::Death);
	}
	if (!HitMontage)
	{
		HitMontage = LoadObject<UAnimMontage>(nullptr, CaveboundMinionPaths::Hit);
	}
	if (!MeleeMontage)
	{
		MeleeMontage = LoadObject<UAnimMontage>(nullptr, CaveboundMinionPaths::Melee);
	}

	// The skeleton needs an UpperBody slot so hit and punch clips can play there.
	if (USkeleton* skeleton = Body->GetSkeletalMeshAsset() ? Body->GetSkeletalMeshAsset()->GetSkeleton() : nullptr)
	{
		static TSet<TWeakObjectPtr<USkeleton>> RegisteredUpperBodySlots;
		if (!RegisteredUpperBodySlots.Contains(skeleton))
		{
			skeleton->RegisterSlotNode(TEXT("UpperBody"));
			RegisteredUpperBodySlots.Add(skeleton);
		}
	}

	// Keep the clip from moving the actor. The path spline already moves it.
	if (UCaveboundMinionAnimInstance* minionAnim = GetMinionAnim())
	{
		minionAnim->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	}
}

void ACaveboundEnemy::BeginPlay()
{
	Super::BeginPlay();
	EnsureAnimationAssets();
}

void ACaveboundEnemy::Tick(float DeltaTime)
{
	// Remember where we were so we can tell if the path move actually happened.
	const FVector locationBeforeMoving = GetActorLocation();
	Super::Tick(DeltaTime);
	UpdatePresentation(DeltaTime, locationBeforeMoving);
}

void ACaveboundEnemy::ApplyDamage(float Amount)
{
	const bool wasAliveBeforeHit = !IsDead();
	Super::ApplyDamage(Amount);

	// A killing blow uses the death animation. A miss on an already dead minion does nothing.
	if (!wasAliveBeforeHit || IsDead())
	{
		return;
	}

	// Flash red, stop the punch, and play the hit on the upper body.
	FCaveboundDamageFlash::FlashComponent(this, Body, BodyFlashTimer, HitFlashColor, HitFlashDuration);

	if (UAnimInstance* minionAnim = GetMinionAnim())
	{
		minionAnim->Montage_Stop(0.12f, MeleeMontage);
	}

	if (!PlayUpperBodyMontage(HitMontage))
	{
		bIsPlayingHitAnimation = false;
	}
}

void ACaveboundEnemy::OnDeath()
{
	// Tell the game this enemy is gone, then stop attacks and play death on the whole body.
	if (UWorld* world = GetWorld())
	{
		if (ACaveboundGameMode* gameMode = world->GetAuthGameMode<ACaveboundGameMode>())
		{
			gameMode->RegisterEnemyDefeated();
		}
	}

	bIsPlayingHitAnimation = false;
	if (UCaveboundMinionAnimInstance* minionAnim = GetMinionAnim())
	{
		minionAnim->Montage_Stop(0.1f);
		minionAnim->SetDead(true);
	}

	const float deathAnimLength = DeathAnim ? DeathAnim->GetPlayLength() : 0.f;
	if (deathAnimLength <= 0.f)
	{
		Destroy();
		return;
	}

	// Wait for the death clip to finish, then delete the actor.
	if (UWorld* world = GetWorld())
	{
		world->GetTimerManager().SetTimer(
			DeathTimer,
			this,
			&ACaveboundEnemy::RemoveAfterDeathAnimation,
			deathAnimLength,
			false);
	}
}

void ACaveboundEnemy::RemoveAfterDeathAnimation()
{
	Destroy();
}

bool ACaveboundEnemy::CanAttack() const
{
	return !bIsPlayingHitAnimation && !IsDead();
}

void ACaveboundEnemy::OnMeleeStrike()
{
	PlayUpperBodyMontage(MeleeMontage);
}

bool ACaveboundEnemy::PlayUpperBodyMontage(UAnimMontage* Montage)
{
	UCaveboundMinionAnimInstance* minionAnim = GetMinionAnim();
	if (!minionAnim || !Montage)
	{
		return false;
	}

	// A play length of zero means the clip did not start.
	if (minionAnim->Montage_Play(Montage, 1.f) <= 0.f)
	{
		return false;
	}

	// While the hit clip is playing, block the next punch. Clear that when the clip ends.
	if (Montage == HitMontage)
	{
		FOnMontageEnded whenHitEnds;
		whenHitEnds.BindUObject(this, &ACaveboundEnemy::HandleHitMontageEnded);
		minionAnim->Montage_SetEndDelegate(whenHitEnds, HitMontage);
		bIsPlayingHitAnimation = true;
	}

	return true;
}

void ACaveboundEnemy::HandleHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == HitMontage)
	{
		bIsPlayingHitAnimation = false;
	}
}

UCaveboundMinionAnimInstance* ACaveboundEnemy::GetMinionAnim() const
{
	return Body ? Cast<UCaveboundMinionAnimInstance>(Body->GetAnimInstance()) : nullptr;
}

void ACaveboundEnemy::UpdatePresentation(float DeltaTime, const FVector& LocationBeforeTick)
{
	UCaveboundMinionAnimInstance* minionAnim = GetMinionAnim();
	if (!minionAnim)
	{
		return;
	}

	// Dead minions stay on the death pose and do not blend back to a walk.
	if (IsDead())
	{
		minionAnim->SetDead(true);
		return;
	}

	minionAnim->SetDead(false);

	// If the actor moved along the path, ease the shown speed toward the real speed.
	const bool enemyMoved = !GetActorLocation().Equals(LocationBeforeTick, 1.f);
	const float speedWeWant = enemyMoved ? MoveSpeed * GetMoveSpeedScale() : 0.f;
	smoothedMoveSpeed = FMath::FInterpTo(smoothedMoveSpeed, speedWeWant, DeltaTime, LocomotionBlendSpeed);
	minionAnim->SetLocomotionSpeed(smoothedMoveSpeed);
}
