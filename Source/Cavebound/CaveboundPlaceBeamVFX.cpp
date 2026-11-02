#include "CaveboundPlaceBeamVFX.h"
#include "CaveboundTurret.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundPlaceBeamVFX::ACaveboundPlaceBeamVFX()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BeamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeamMesh"));
	BeamMesh->SetupAttachment(Root);
	BeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BeamMesh->SetCastShadow(false);
	BeamMesh->SetReceivesDecals(false);

	PopMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PopMesh"));
	PopMesh->SetupAttachment(Root);
	PopMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PopMesh->SetCastShadow(false);
	PopMesh->SetReceivesDecals(false);
	PopMesh->SetVisibility(false);
	PopMesh->SetRelativeScale3D(FVector(0.05f));

	Niagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Niagara"));
	Niagara->SetupAttachment(Root);
	Niagara->SetAutoActivate(false);

	PopNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PopNiagara"));
	PopNiagara->SetupAttachment(Root);
	PopNiagara->SetAutoActivate(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderFinder.Succeeded())
	{
		BeamMesh->SetStaticMesh(CylinderFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereFinder.Succeeded())
	{
		PopMesh->SetStaticMesh(SphereFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> EmissiveFinder(
		TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	if (EmissiveFinder.Succeeded())
	{
		BeamMesh->SetMaterial(0, EmissiveFinder.Object);
		PopMesh->SetMaterial(0, EmissiveFinder.Object);
	}

	BeamSystem = TSoftObjectPtr<UNiagaraSystem>(
		FSoftObjectPath(TEXT("/Game/Assets/Effects/Particles/NS_PlaceBeam.NS_PlaceBeam")));
	PopSystem = TSoftObjectPtr<UNiagaraSystem>(
		FSoftObjectPath(TEXT("/Game/Assets/Effects/Particles/NS_PlacePop.NS_PlacePop")));
}

FLinearColor ACaveboundPlaceBeamVFX::ColorForBuild()
{
	return FLinearColor(1.f, 0.85f, 0.12f, 1.f);
}

FLinearColor ACaveboundPlaceBeamVFX::ColorForUpgrade(int32 NextUpgradeLevel)
{
	if (NextUpgradeLevel >= 2)
	{
		return FLinearColor(1.f, 0.18f, 0.12f, 1.f);
	}
	return FLinearColor(0.25f, 0.9f, 1.f, 1.f);
}

ACaveboundPlaceBeamVFX* ACaveboundPlaceBeamVFX::SpawnAt(
	UWorld* World,
	const FVector& Location,
	const FLinearColor& Color,
	AActor* InRevealActor,
	TFunction<void()> Finished)
{
	if (!World)
	{
		return nullptr;
	}

	const FTransform SpawnTransform(FRotator::ZeroRotator, Location);
	ACaveboundPlaceBeamVFX* Beam = World->SpawnActorDeferred<ACaveboundPlaceBeamVFX>(
		ACaveboundPlaceBeamVFX::StaticClass(),
		SpawnTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Beam)
	{
		return nullptr;
	}

	Beam->BeamColor = Color;
	Beam->RevealActor = InRevealActor;
	Beam->OnFinished = MoveTemp(Finished);
	return Cast<ACaveboundPlaceBeamVFX>(UGameplayStatics::FinishSpawningActor(Beam, SpawnTransform));
}

void ACaveboundPlaceBeamVFX::BeginPlay()
{
	Super::BeginPlay();

	Age = 0.f;
	PopAge = 0.f;
	bInPop = false;
	bFinished = false;

	if (UMaterialInterface* BaseMat = BeamMesh ? BeamMesh->GetMaterial(0) : nullptr)
	{
		BeamMID = BeamMesh->CreateDynamicMaterialInstance(0, BaseMat);
		if (BeamMID)
		{
			BeamMID->SetVectorParameterValue(TEXT("Color"), BeamColor);
			BeamMID->SetVectorParameterValue(TEXT("EmissiveColor"), BeamColor);
		}
	}

	if (UMaterialInterface* PopBase = PopMesh ? PopMesh->GetMaterial(0) : nullptr)
	{
		PopMID = PopMesh->CreateDynamicMaterialInstance(0, PopBase);
		if (PopMID)
		{
			PopMID->SetVectorParameterValue(TEXT("Color"), BeamColor);
			PopMID->SetVectorParameterValue(TEXT("EmissiveColor"), BeamColor);
		}
	}

	const float RadiusScale = FMath::Max(1.f, BeamRadius) / 50.f;
	BeamMesh->SetRelativeLocation(FVector(0.f, 0.f, BeamHeight * 0.5f));
	BeamMesh->SetRelativeScale3D(FVector(RadiusScale, RadiusScale, 0.05f));

	UNiagaraSystem* System = BeamSystem.LoadSynchronous();
	if (!System)
	{
		System = LoadObject<UNiagaraSystem>(
			nullptr,
			TEXT("/Niagara/DefaultAssets/Templates/Systems/FountainLightweight.FountainLightweight"));
	}

	if (System)
	{
		Niagara->SetAsset(System);
		Niagara->SetRelativeScale3D(FVector(FMath::Max(0.55f, BeamRadius / 28.f)));
		Niagara->Activate(true);
	}
	else
	{
		Niagara->Deactivate();
	}

	BeginReveal();
	SetActorTickEnabled(true);
}

void ACaveboundPlaceBeamVFX::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearReveal();
	Super::EndPlay(EndPlayReason);
}

void ACaveboundPlaceBeamVFX::BeginReveal()
{
	AActor* Actor = RevealActor.Get();
	if (!Actor)
	{
		return;
	}

	if (ACaveboundTurret* Turret = Cast<ACaveboundTurret>(Actor))
	{
		Turret->SetRevealLocked(true);
	}

	Actor->SetActorHiddenInGame(false);

	TArray<UStaticMeshComponent*> Meshes;
	Actor->GetComponents<UStaticMeshComponent>(Meshes);
	RevealMeshes.Reset();
	RevealBaseScales.Reset();

	UMaterialInterface* OverlayBase = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	if (OverlayBase)
	{
		RevealMID = UMaterialInstanceDynamic::Create(OverlayBase, this);
	}

	for (UStaticMeshComponent* Mesh : Meshes)
	{
		if (!IsValid(Mesh) || !Mesh->GetStaticMesh())
		{
			continue;
		}

		RevealMeshes.Add(Mesh);
		RevealBaseScales.Add(Mesh->GetRelativeScale3D());
		Mesh->SetVisibility(true);
		Mesh->SetHiddenInGame(false);
		if (RevealMID)
		{
			Mesh->SetOverlayMaterial(RevealMID);
		}
	}

	UpdateReveal(0.f);
}

void ACaveboundPlaceBeamVFX::UpdateReveal(float Alpha)
{
	const float Clamped = FMath::Clamp(Alpha, 0.f, 1.f);
	const float Soft = FMath::InterpEaseOut(0.f, 1.f, Clamped, 2.f);

	if (RevealMID)
	{
		// Strong beam-colored hologram that eases toward the real mesh.
		FLinearColor Tint = BeamColor;
		Tint.A = FMath::Lerp(0.95f, 0.15f, Soft);
		RevealMID->SetVectorParameterValue(TEXT("Color"), Tint);
	}

	for (int32 Index = 0; Index < RevealMeshes.Num(); ++Index)
	{
		UStaticMeshComponent* Mesh = RevealMeshes[Index].Get();
		if (!IsValid(Mesh))
		{
			continue;
		}

		const FVector BaseScale = RevealBaseScales.IsValidIndex(Index)
			? RevealBaseScales[Index]
			: FVector::OneVector;
		const float ScaleAlpha = FMath::Lerp(0.2f, 1.f, Soft);
		Mesh->SetRelativeScale3D(BaseScale * ScaleAlpha);
	}
}

void ACaveboundPlaceBeamVFX::ClearReveal()
{
	for (int32 Index = 0; Index < RevealMeshes.Num(); ++Index)
	{
		UStaticMeshComponent* Mesh = RevealMeshes[Index].Get();
		if (!IsValid(Mesh))
		{
			continue;
		}

		Mesh->SetOverlayMaterial(nullptr);
		if (RevealBaseScales.IsValidIndex(Index))
		{
			Mesh->SetRelativeScale3D(RevealBaseScales[Index]);
		}
	}

	if (ACaveboundTurret* Turret = Cast<ACaveboundTurret>(RevealActor.Get()))
	{
		Turret->SetRevealLocked(false);
	}

	RevealMeshes.Reset();
	RevealBaseScales.Reset();
	RevealMID = nullptr;
}

void ACaveboundPlaceBeamVFX::StartPop()
{
	bInPop = true;
	PopAge = 0.f;

	if (Niagara)
	{
		Niagara->Deactivate();
	}
	if (BeamMesh)
	{
		BeamMesh->SetVisibility(false);
	}

	// Finish the materialize: real mesh, full scale, then the pop sells the spawn-in.
	ClearReveal();
	if (OnFinished)
	{
		TFunction<void()> Callback = MoveTemp(OnFinished);
		OnFinished = nullptr;
		Callback();
	}

	if (PopMesh)
	{
		PopMesh->SetVisibility(true);
		PopMesh->SetRelativeScale3D(FVector(0.08f));
	}

	UNiagaraSystem* System = PopSystem.LoadSynchronous();
	if (!System)
	{
		System = LoadObject<UNiagaraSystem>(
			nullptr,
			TEXT("/Niagara/DefaultAssets/Templates/Systems/RadialBurst.RadialBurst"));
	}
	if (!System)
	{
		System = LoadObject<UNiagaraSystem>(
			nullptr,
			TEXT("/Niagara/DefaultAssets/Templates/Systems/SimpleExplosion.SimpleExplosion"));
	}

	if (System && PopNiagara)
	{
		PopNiagara->SetAsset(System);
		PopNiagara->SetRelativeScale3D(FVector(1.35f));
		PopNiagara->Activate(true);
	}
}

void ACaveboundPlaceBeamVFX::FinishAndDestroy()
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;

	if (Niagara)
	{
		Niagara->Deactivate();
	}
	if (PopNiagara)
	{
		PopNiagara->Deactivate();
	}

	ClearReveal();
	Destroy();
}

void ACaveboundPlaceBeamVFX::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bInPop)
	{
		PopAge += DeltaTime;
		const float T = FMath::Clamp(PopAge / FMath::Max(0.05f, PopDuration), 0.f, 1.f);
		const float PopScale = FMath::InterpEaseOut(0.08f, 2.4f, T, 2.f);
		const float PopAlpha = 1.f - T;

		if (PopMesh)
		{
			PopMesh->SetRelativeScale3D(FVector(PopScale));
			PopMesh->SetVisibility(PopAlpha > 0.02f);
		}
		if (PopMID)
		{
			const FLinearColor Lit = BeamColor * (0.5f + 2.2f * PopAlpha);
			PopMID->SetVectorParameterValue(TEXT("Color"), Lit);
			PopMID->SetVectorParameterValue(TEXT("EmissiveColor"), Lit);
			PopMID->SetScalarParameterValue(TEXT("Opacity"), PopAlpha);
			PopMID->SetScalarParameterValue(TEXT("Multiply"), PopAlpha);
		}

		if (T >= 1.f)
		{
			FinishAndDestroy();
		}
		return;
	}

	Age += DeltaTime;
	const float T = FMath::Clamp(Age / FMath::Max(0.05f, Duration), 0.f, 1.f);

	const float Rise = FMath::InterpEaseOut(0.05f, 1.f, FMath::Clamp(T / 0.22f, 0.f, 1.f), 2.f);
	const float Fade = T < 0.55f ? 1.f : FMath::InterpEaseIn(1.f, 0.f, (T - 0.55f) / 0.45f, 2.f);
	const float Alpha = Rise * Fade;

	const float HeightScale = (FMath::Max(10.f, BeamHeight) / 100.f) * Rise;
	const float RadiusScale = (FMath::Max(1.f, BeamRadius) / 50.f) * FMath::Lerp(1.2f, 0.65f, T);
	BeamMesh->SetRelativeLocation(FVector(0.f, 0.f, BeamHeight * 0.5f * Rise));
	BeamMesh->SetRelativeScale3D(FVector(RadiusScale, RadiusScale, HeightScale));
	BeamMesh->SetVisibility(Alpha > 0.02f);

	if (BeamMID)
	{
		const FLinearColor Lit = BeamColor * (0.35f + 1.8f * Alpha);
		BeamMID->SetVectorParameterValue(TEXT("Color"), Lit);
		BeamMID->SetVectorParameterValue(TEXT("EmissiveColor"), Lit);
		BeamMID->SetScalarParameterValue(TEXT("Opacity"), Alpha);
		BeamMID->SetScalarParameterValue(TEXT("Multiply"), Alpha);
	}

	// Defender fades in over most of the beam, fully formed before the pop.
	UpdateReveal(FMath::Clamp(T / 0.85f, 0.f, 1.f));

	if (T >= 1.f)
	{
		StartPop();
	}
}
