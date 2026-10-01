#include "CaveboundMinionAnimInstance.h"
#include "Animation/AnimClassInterface.h"
#include "AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "AnimNodes/AnimNode_LayeredBoneBlend.h"
#include "AnimNodes/AnimNode_Slot.h"

void UCaveboundMinionAnimInstance::SetLocomotionSpeed(float Speed)
{
	LocomotionSpeed = Speed;
}

void UCaveboundMinionAnimInstance::SetDead(bool bDead)
{
	bIsDead = !bDead;
}

void UCaveboundMinionAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	bIsDead = true;
	EnsureUpperBodyLayer();
}

void UCaveboundMinionAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	EnsureUpperBodyLayer();
	Super::NativeUpdateAnimation(DeltaSeconds);
}

void UCaveboundMinionAnimInstance::EnsureUpperBodyLayer()
{
	IAnimClassInterface* AnimClass = IAnimClassInterface::GetFromClass(GetClass());
	if (!AnimClass)
	{
		return;
	}

	FAnimNode_BlendSpacePlayerBase* Locomotion = nullptr;
	FAnimNode_Slot* UpperSlot = nullptr;
	FAnimNode_LayeredBoneBlend* Layer = nullptr;

	for (const FStructProperty* Property : AnimClass->GetAnimNodeProperties())
	{
		if (!Property || !Property->Struct)
		{
			continue;
		}

		if (Property->Struct->IsChildOf(FAnimNode_BlendSpacePlayerBase::StaticStruct()))
		{
			Locomotion = Property->ContainerPtrToValuePtr<FAnimNode_BlendSpacePlayerBase>(this);
		}
		else if (Property->Struct == FAnimNode_Slot::StaticStruct())
		{
			FAnimNode_Slot* SlotNode = Property->ContainerPtrToValuePtr<FAnimNode_Slot>(this);
			if (SlotNode && SlotNode->SlotName == TEXT("UpperBody"))
			{
				UpperSlot = SlotNode;
			}
		}
		else if (Property->Struct == FAnimNode_LayeredBoneBlend::StaticStruct())
		{
			Layer = Property->ContainerPtrToValuePtr<FAnimNode_LayeredBoneBlend>(this);
		}
	}

	if (UpperSlot)
	{
		UpperSlot->bAlwaysUpdateSourcePose = true;
		if (Locomotion && UpperSlot->Source.GetLinkNode() != Locomotion)
		{
			UpperSlot->Source.SetLinkNode(Locomotion);
		}
	}

	if (!Layer)
	{
		return;
	}

	if (Layer->BlendPoses.Num() == 0)
	{
		if (Layer->BlendWeights.Num() != 0 || Layer->LayerSetup.Num() != 0)
		{
			Layer->BlendWeights.Reset();
			Layer->LayerSetup.Reset();
			Layer->InvalidatePerBoneBlendWeights();
		}
		return;
	}

	if (UpperSlot && Layer->BlendPoses[0].GetLinkNode() != UpperSlot)
	{
		Layer->BlendPoses[0].SetLinkNode(UpperSlot);
	}

	const int32 NumPoses = Layer->BlendPoses.Num();
	bool bChanged = Layer->LayerSetup.Num() != NumPoses || Layer->BlendWeights.Num() != NumPoses;
	Layer->LayerSetup.SetNum(NumPoses);
	Layer->BlendWeights.SetNum(NumPoses);

	const bool bOverlayActive = IsSlotActive(TEXT("UpperBody"));
	for (int32 Index = 0; Index < Layer->BlendWeights.Num(); ++Index)
	{
		const float DesiredWeight = (Index == 0 && bOverlayActive) ? 1.f : 0.f;
		if (!FMath::IsNearlyEqual(Layer->BlendWeights[Index], DesiredWeight))
		{
			Layer->BlendWeights[Index] = DesiredWeight;
		}
	}

	FInputBlendPose& UpperBody = Layer->LayerSetup[0];
	const bool bFilterWrong = UpperBody.BranchFilters.Num() != 1
		|| UpperBody.BranchFilters[0].BoneName != TEXT("spine")
		|| UpperBody.BranchFilters[0].BlendDepth != 0;
	if (bFilterWrong)
	{
		UpperBody.BranchFilters.Reset();
		FBranchFilter Filter;
		Filter.BoneName = TEXT("spine");
		Filter.BlendDepth = 0;
		UpperBody.BranchFilters.Add(Filter);
		bChanged = true;
	}

	for (int32 Index = 1; Index < Layer->LayerSetup.Num(); ++Index)
	{
		if (Layer->LayerSetup[Index].BranchFilters.Num() > 0)
		{
			Layer->LayerSetup[Index].BranchFilters.Reset();
			bChanged = true;
		}
	}

	if (bChanged)
	{
		Layer->InvalidatePerBoneBlendWeights();
	}
}
