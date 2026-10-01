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
	// Keep the actor origin on the path. The minion's feet are at the mesh origin.
	PathHeightOffset = 0.f;

	VisualMesh->SetVisibility(false);
	VisualMesh->SetHiddenInGame(true);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetCastShadow(false);

	HitVolume = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HitVolume"));
	HitVolume->SetupAttachment(VisualMesh);
	HitVolume->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	HitVolume->SetRelativeScale3D(FVector(0.8f, 0.5f, 2.0f));
	HitVolume->SetVisibility(false);
	HitVolume->SetHiddenInGame(true);
	HitVolume->SetCastShadow(false);
	HitVolume->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HitVolume->SetCollisionObjectType(ECC_WorldDynamic);
	HitVolume->SetCollisionResponseToAllChannels(ECR_Block);
	HitVolume->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	HitVolume->SetGenerateOverlapEvents(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		HitVolume->SetStaticMesh(CubeMesh.Object);
	}

	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(VisualMesh);
	Body->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(CaveboundMinionPaths::Mesh);
	if (MeshAsset.Succeeded())
	{
		BodyMesh = MeshAsset.Object;
		Body->SetSkeletalMeshAsset(BodyMesh);
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
}

void ACaveboundEnemy::EnsureAnimationAssets()
{
	if (!Body)
	{
		return;
	}

	if (!BodyMesh)
	{
		BodyMesh = LoadObject<USkeletalMesh>(nullptr, CaveboundMinionPaths::Mesh);
	}
	if (BodyMesh)
	{
		Body->SetSkeletalMeshAsset(BodyMesh);
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

	if (USkeleton* Skeleton = Body->GetSkeletalMeshAsset() ? Body->GetSkeletalMeshAsset()->GetSkeleton() : nullptr)
	{
		Skeleton->RegisterSlotNode(TEXT("UpperBody"));
	}

	if (UCaveboundMinionAnimInstance* Anim = GetMinionAnim())
	{
		Anim->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
	}
}

void ACaveboundEnemy::BeginPlay()
{
	Super::BeginPlay();
	EnsureAnimationAssets();
}

void ACaveboundEnemy::Tick(float DeltaTime)
{
	const FVector LocationBeforeTick = GetActorLocation();
	Super::Tick(DeltaTime);
	UpdatePresentation(DeltaTime, LocationBeforeTick);
}

void ACaveboundEnemy::ApplyDamage(float Amount)
{
	const bool bWasAlive = !IsDead();
	Super::ApplyDamage(Amount);

	if (!bWasAlive || IsDead())
	{
		return;
	}

	FCaveboundDamageFlash::FlashComponent(this, Body, BodyFlashTimer, HitFlashColor, HitFlashDuration);

	if (UAnimInstance* Anim = GetMinionAnim())
	{
		Anim->Montage_Stop(0.12f, MeleeMontage);
	}

	if (!PlayUpperBodyMontage(HitMontage))
	{
		bHitReacting = false;
	}
}

void ACaveboundEnemy::OnDeath()
{
	if (UWorld* World = GetWorld())
	{
		if (ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>())
		{
			GameMode->RegisterEnemyDefeated();
		}
	}

	bHitReacting = false;
	if (UCaveboundMinionAnimInstance* Anim = GetMinionAnim())
	{
		Anim->Montage_Stop(0.1f);
		Anim->SetDead(true);
	}

	const float DeathLength = DeathAnim ? DeathAnim->GetPlayLength() : 0.f;
	if (DeathLength <= 0.f)
	{
		Destroy();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DeathTimer,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				Destroy();
			}),
			DeathLength,
			false);
	}
}

bool ACaveboundEnemy::CanAttack() const
{
	return !bHitReacting && !IsDead();
}

void ACaveboundEnemy::OnMeleeStrike()
{
	PlayUpperBodyMontage(MeleeMontage);
}

bool ACaveboundEnemy::PlayUpperBodyMontage(UAnimMontage* Montage)
{
	UCaveboundMinionAnimInstance* Anim = GetMinionAnim();
	if (!Anim || !Montage)
	{
		return false;
	}

	if (Anim->Montage_Play(Montage, 1.f) <= 0.f)
	{
		return false;
	}

	if (Montage == HitMontage)
	{
		FOnMontageEnded Ended;
		Ended.BindUObject(this, &ACaveboundEnemy::HandleHitMontageEnded);
		Anim->Montage_SetEndDelegate(Ended, HitMontage);
		bHitReacting = true;
	}

	return true;
}

void ACaveboundEnemy::HandleHitMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == HitMontage)
	{
		bHitReacting = false;
	}
}

UCaveboundMinionAnimInstance* ACaveboundEnemy::GetMinionAnim() const
{
	return Body ? Cast<UCaveboundMinionAnimInstance>(Body->GetAnimInstance()) : nullptr;
}

void ACaveboundEnemy::UpdatePresentation(float DeltaTime, const FVector& LocationBeforeTick)
{
	UCaveboundMinionAnimInstance* Anim = GetMinionAnim();
	if (!Anim)
	{
		return;
	}

	if (IsDead())
	{
		Anim->SetDead(true);
		return;
	}

	Anim->SetDead(false);

	const bool bMoved = !GetActorLocation().Equals(LocationBeforeTick, 1.f);
	const float TargetSpeed = bMoved ? MoveSpeed * GetMoveSpeedScale() : 0.f;
	SmoothedSpeed = FMath::FInterpTo(SmoothedSpeed, TargetSpeed, DeltaTime, LocomotionBlendSpeed);
	Anim->SetLocomotionSpeed(SmoothedSpeed);
}
