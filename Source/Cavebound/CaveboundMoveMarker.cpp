#include "CaveboundMoveMarker.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundMoveMarker::ACaveboundMoveMarker()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Two flat discs, offset in pulse, read as a ring.
	Outer = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Outer"));
	Outer->SetupAttachment(Root);
	Outer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Outer->SetCastShadow(false);
	Outer->SetReceivesDecals(false);

	Inner = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Inner"));
	Inner->SetupAttachment(Root);
	Inner->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Inner->SetCastShadow(false);
	Inner->SetReceivesDecals(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		Outer->SetStaticMesh(Cylinder.Object);
		Inner->SetStaticMesh(Cylinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMat(
		TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	if (GlowMat.Succeeded())
	{
		Outer->SetMaterial(0, GlowMat.Object);
		Inner->SetMaterial(0, GlowMat.Object);
	}

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void ACaveboundMoveMarker::ShowAt(const FVector& WorldLocation)
{
	SetActorLocation(WorldLocation + FVector(0.f, 0.f, GroundOffset));
	SetActorHiddenInGame(false);
	bVisible = true;
	PulseTime = 0.f;

	if (!OuterMat && Outer)
	{
		if (UMaterialInterface* Base = Outer->GetMaterial(0))
		{
			OuterMat = Outer->CreateDynamicMaterialInstance(0, Base);
		}
	}
	if (!InnerMat && Inner)
	{
		if (UMaterialInterface* Base = Inner->GetMaterial(0))
		{
			InnerMat = Inner->CreateDynamicMaterialInstance(0, Base);
		}
	}

	UpdatePulse(0.f);
	SetActorTickEnabled(true);
}

void ACaveboundMoveMarker::Hide()
{
	bVisible = false;
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);
}

void ACaveboundMoveMarker::UpdatePulse(float Amount)
{
	// Outer grows as Inner shrinks so it reads as a ring, not a solid disc.
	const float OuterScale = FMath::Lerp(0.85f, 1.25f, Amount);
	const float InnerScale = FMath::Lerp(1.15f, 0.7f, Amount);
	const float R = Radius / 50.f;
	const float H = 0.03f;

	Outer->SetRelativeScale3D(FVector(R * OuterScale, R * OuterScale, H));
	Inner->SetRelativeScale3D(FVector(R * InnerScale * 0.55f, R * InnerScale * 0.55f, H * 1.2f));

	const float OuterGlow = FMath::Lerp(0.55f, 1.f, Amount);
	const float InnerGlow = FMath::Lerp(1.f, 0.35f, Amount);

	if (OuterMat)
	{
		const FLinearColor Lit = Color * (0.4f + 1.6f * OuterGlow);
		OuterMat->SetVectorParameterValue(TEXT("Color"), Lit);
		OuterMat->SetVectorParameterValue(TEXT("EmissiveColor"), Lit);
		OuterMat->SetScalarParameterValue(TEXT("Opacity"), OuterGlow);
		OuterMat->SetScalarParameterValue(TEXT("Multiply"), OuterGlow);
	}
	if (InnerMat)
	{
		const FLinearColor Lit = Color * (0.5f + 1.4f * InnerGlow);
		InnerMat->SetVectorParameterValue(TEXT("Color"), Lit);
		InnerMat->SetVectorParameterValue(TEXT("EmissiveColor"), Lit);
		InnerMat->SetScalarParameterValue(TEXT("Opacity"), InnerGlow);
		InnerMat->SetScalarParameterValue(TEXT("Multiply"), InnerGlow);
	}
}

void ACaveboundMoveMarker::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bVisible)
	{
		return;
	}

	PulseTime += DeltaTime * PulseSpeed;
	UpdatePulse(0.5f + 0.5f * FMath::Sin(PulseTime));
}
