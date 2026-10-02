#include "CaveboundMinionAnimInstance.h"
#include "Animation/AnimClassInterface.h"
#include "AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "AnimNodes/AnimNode_LayeredBoneBlend.h"
#include "AnimNodes/AnimNode_Slot.h"

void UCaveboundMinionAnimInstance::SetLocomotionSpeed(float Speed)
{
	// Save the speed so the walk blend space can read it.
	LocomotionSpeed = Speed;
}

void UCaveboundMinionAnimInstance::SetDead(bool bDead)
{
	// The blend node treats true as the walk pose and false as death.
	// Flip the value so SetDead(true) actually plays death.
	bIsDead = !bDead;
}

void UCaveboundMinionAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// Start on the walk pose, then make sure the upper body is set up.
	bIsDead = true;
	EnsureUpperBodyLayer();
}

void UCaveboundMinionAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	// Fix the upper body every frame before the animation plays.
	EnsureUpperBodyLayer();
	Super::NativeUpdateAnimation(DeltaSeconds);
}

void UCaveboundMinionAnimInstance::EnsureUpperBodyLayer()
{
	// Find the animation nodes that the blueprint already created.
	IAnimClassInterface* animationClass = IAnimClassInterface::GetFromClass(GetClass());
	if (!animationClass)
	{
		return;
	}

	FAnimNode_BlendSpacePlayerBase* walkAnimNode = nullptr;
	FAnimNode_Slot* upperBodySlot = nullptr;
	FAnimNode_LayeredBoneBlend* upperBodyBlend = nullptr;

	for (const FStructProperty* nodeProperty : animationClass->GetAnimNodeProperties())
	{
		if (!nodeProperty || !nodeProperty->Struct)
		{
			continue;
		}

		// The first node is the idle / walk / run blend.
		if (nodeProperty->Struct->IsChildOf(FAnimNode_BlendSpacePlayerBase::StaticStruct()))
		{
			walkAnimNode = nodeProperty->ContainerPtrToValuePtr<FAnimNode_BlendSpacePlayerBase>(this);
		}
		// The slot named UpperBody is where hit and punch animations play.
		else if (nodeProperty->Struct == FAnimNode_Slot::StaticStruct())
		{
			FAnimNode_Slot* foundSlot = nodeProperty->ContainerPtrToValuePtr<FAnimNode_Slot>(this);
			if (foundSlot && foundSlot->SlotName == TEXT("UpperBody"))
			{
				upperBodySlot = foundSlot;
			}
		}
		// This blend puts the slot on the spine and leaves the legs on the walk.
		else if (nodeProperty->Struct == FAnimNode_LayeredBoneBlend::StaticStruct())
		{
			upperBodyBlend = nodeProperty->ContainerPtrToValuePtr<FAnimNode_LayeredBoneBlend>(this);
		}
	}

	// Keep the slot's source as the walk animation when nothing else is playing.
	if (upperBodySlot)
	{
		upperBodySlot->bAlwaysUpdateSourcePose = true;
		if (walkAnimNode && upperBodySlot->Source.GetLinkNode() != walkAnimNode)
		{
			upperBodySlot->Source.SetLinkNode(walkAnimNode);
		}
	}

	if (!upperBodyBlend)
	{
		return;
	}

	// If there is no pose to blend, clear leftover weights so the game does not crash.
	if (upperBodyBlend->BlendPoses.Num() == 0)
	{
		if (upperBodyBlend->BlendWeights.Num() != 0 || upperBodyBlend->LayerSetup.Num() != 0)
		{
			upperBodyBlend->BlendWeights.Reset();
			upperBodyBlend->LayerSetup.Reset();
			upperBodyBlend->InvalidatePerBoneBlendWeights();
		}
		return;
	}

	// The first blend pose should be the upper body slot.
	if (upperBodySlot && upperBodyBlend->BlendPoses[0].GetLinkNode() != upperBodySlot)
	{
		upperBodyBlend->BlendPoses[0].SetLinkNode(upperBodySlot);
	}

	// One weight and one bone filter for each pose.
	const int32 numberOfPoses = upperBodyBlend->BlendPoses.Num();
	bool boneSetupChanged = upperBodyBlend->LayerSetup.Num() != numberOfPoses || upperBodyBlend->BlendWeights.Num() != numberOfPoses;
	upperBodyBlend->LayerSetup.SetNum(numberOfPoses);
	upperBodyBlend->BlendWeights.SetNum(numberOfPoses);

	// Show the hit or punch only while that slot is playing. Otherwise the upper body keeps walking.
	const bool bHitOrAttackIsPlaying = IsSlotActive(TEXT("UpperBody"));
	for (int32 poseIndex = 0; poseIndex < upperBodyBlend->BlendWeights.Num(); ++poseIndex)
	{
		const float wantedWeight = (poseIndex == 0 && bHitOrAttackIsPlaying) ? 1.f : 0.f;
		if (!FMath::IsNearlyEqual(upperBodyBlend->BlendWeights[poseIndex], wantedWeight))
		{
			upperBodyBlend->BlendWeights[poseIndex] = wantedWeight;
		}
	}

	// Only bones from the spine upward use the hit or punch. The legs stay on the walk.
	FInputBlendPose& upperBodyBones = upperBodyBlend->LayerSetup[0];
	const bool spineFilterIsWrong = upperBodyBones.BranchFilters.Num() != 1
		|| upperBodyBones.BranchFilters[0].BoneName != TEXT("spine")
		|| upperBodyBones.BranchFilters[0].BlendDepth != 0;
	if (spineFilterIsWrong)
	{
		upperBodyBones.BranchFilters.Reset();
		FBranchFilter spineFilter;
		spineFilter.BoneName = TEXT("spine");
		spineFilter.BlendDepth = 0;
		upperBodyBones.BranchFilters.Add(spineFilter);
		boneSetupChanged = true;
	}

	// Extra layers are unused. Clear them so they cannot cover the whole body.
	for (int32 poseIndex = 1; poseIndex < upperBodyBlend->LayerSetup.Num(); ++poseIndex)
	{
		if (upperBodyBlend->LayerSetup[poseIndex].BranchFilters.Num() > 0)
		{
			upperBodyBlend->LayerSetup[poseIndex].BranchFilters.Reset();
			boneSetupChanged = true;
		}
	}

	if (boneSetupChanged)
	{
		upperBodyBlend->InvalidatePerBoneBlendWeights();
	}
}
