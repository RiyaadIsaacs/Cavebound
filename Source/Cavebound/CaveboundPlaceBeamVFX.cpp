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

	// Tall cylinder = the beam body.
	BeamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeamMesh"));
	BeamMesh->SetupAttachment(Root);
	BeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BeamMesh->SetCastShadow(false);
	BeamMesh->SetReceivesDecals(false);

	// Sphere flash at the end of the cast.
	PopMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PopMesh"));
	PopMesh->SetupAttachment(Root);
	PopMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PopMesh->SetCastShadow(false);
	PopMesh->SetReceivesDecals(false);
	PopMesh->SetVisibility(false);
	PopMesh->SetRelativeScale3D(FVector(0.05f));

	BeamNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BeamNiagara"));
	BeamNiagara->SetupAttachment(Root);
	BeamNiagara->SetAutoActivate(false);

	PopNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PopNiagara"));
	PopNiagara->SetupAttachment(Root);
	PopNiagara->SetAutoActivate(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		BeamMesh->SetStaticMesh(Cylinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded())
	{
		PopMesh->SetStaticMesh(Sphere.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMat(
		TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	if (GlowMat.Succeeded())
	{
		BeamMesh->SetMaterial(0, GlowMat.Object);
		PopMesh->SetMaterial(0, GlowMat.Object);
	}

	// Optional Niagara assets — if unset, BeginPlay / StartPop use engine fallbacks.
}

FLinearColor ACaveboundPlaceBeamVFX::ColorForBuild()
{
	return FLinearColor(1.f, 0.85f, 0.12f, 1.f); // yellow
}

FLinearColor ACaveboundPlaceBeamVFX::ColorForUpgrade(int32 NextUpgradeLevel)
{
	if (NextUpgradeLevel >= 2)
	{
		return FLinearColor(1.f, 0.18f, 0.12f, 1.f); // red — 2nd upgrade
	}
	return FLinearColor(0.25f, 0.9f, 1.f, 1.f); // cyan — 1st upgrade
}

ACaveboundPlaceBeamVFX* ACaveboundPlaceBeamVFX::SpawnAt(
	UWorld* World,
	const FVector& Location,
	const FLinearColor& Color,
	AActor* InRevealActor,
	bool bInFadeIn,
	TFunction<void()> InOnDone)
{
	if (!World)
	{
		return nullptr;
	}

	const FTransform SpawnXform(FRotator::ZeroRotator, Location);
	ACaveboundPlaceBeamVFX* Beam = World->SpawnActorDeferred<ACaveboundPlaceBeamVFX>(
		ACaveboundPlaceBeamVFX::StaticClass(),
		SpawnXform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Beam)
	{
		return nullptr;
	}

	Beam->BeamColor = Color;
	Beam->RevealActor = InRevealActor;
	Beam->bFadeIn = bInFadeIn;
	Beam->OnDone = MoveTemp(InOnDone);
	return Cast<ACaveboundPlaceBeamVFX>(UGameplayStatics::FinishSpawningActor(Beam, SpawnXform));
}

void ACaveboundPlaceBeamVFX::BeginPlay()
{
	Super::BeginPlay();

	Time = 0.f;
	PopTime = 0.f;
	bPopping = false;
	bDone = false;

	// Tint the beam mesh.
	if (UMaterialInterface* Base = BeamMesh ? BeamMesh->GetMaterial(0) : nullptr)
	{
		BeamMat = BeamMesh->CreateDynamicMaterialInstance(0, Base);
		if (BeamMat)
		{
			BeamMat->SetVectorParameterValue(TEXT("Color"), BeamColor);
			BeamMat->SetVectorParameterValue(TEXT("EmissiveColor"), BeamColor);
		}
	}

	if (UMaterialInterface* Base = PopMesh ? PopMesh->GetMaterial(0) : nullptr)
	{
		PopMat = PopMesh->CreateDynamicMaterialInstance(0, Base);
		if (PopMat)
		{
			PopMat->SetVectorParameterValue(TEXT("Color"), BeamColor);
			PopMat->SetVectorParameterValue(TEXT("EmissiveColor"), BeamColor);
		}
	}

	// Start small; Tick grows the beam upward.
	const float Radius = FMath::Max(1.f, BeamRadius) / 50.f;
	BeamMesh->SetRelativeLocation(FVector(0.f, 0.f, BeamHeight * 0.5f));
	BeamMesh->SetRelativeScale3D(FVector(Radius, Radius, 0.05f));

	// Optional Niagara particles on top of the mesh beam.
	UNiagaraSystem* System = BeamSystem.LoadSynchronous();
	if (!System)
	{
		System = LoadObject<UNiagaraSystem>(
			nullptr,
			TEXT("/Niagara/DefaultAssets/Templates/Systems/FountainLightweight.FountainLightweight"));
	}

	if (System)
	{
		BeamNiagara->SetAsset(System);
		BeamNiagara->SetRelativeScale3D(FVector(FMath::Max(0.55f, BeamRadius / 28.f)));
		BeamNiagara->Activate(true);
	}
	else
	{
		BeamNiagara->Deactivate();
	}

	StartReveal();
	SetActorTickEnabled(true);
}

void ACaveboundPlaceBeamVFX::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearReveal();
	Super::EndPlay(EndPlayReason);
}

void ACaveboundPlaceBeamVFX::StartReveal()
{
	AActor* Actor = RevealActor.Get();
	if (!Actor)
	{
		return;
	}

	// Keep turret from fighting the fade while we own its look.
	if (ACaveboundTurret* Turret = Cast<ACaveboundTurret>(Actor))
	{
		Turret->SetRevealLocked(true);
	}

	Actor->SetActorHiddenInGame(false);

	TArray<UStaticMeshComponent*> Found;
	Actor->GetComponents<UStaticMeshComponent>(Found);
	Meshes.Reset();
	MeshScales.Reset();

	UMaterialInterface* OverlayBase = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	if (OverlayBase)
	{
		OverlayMat = UMaterialInstanceDynamic::Create(OverlayBase, this);
	}

	for (UStaticMeshComponent* Mesh : Found)
	{
		if (!IsValid(Mesh) || !Mesh->GetStaticMesh())
		{
			continue;
		}

		Meshes.Add(Mesh);
		MeshScales.Add(Mesh->GetRelativeScale3D());
		Mesh->SetVisibility(true);
		Mesh->SetHiddenInGame(false);
		if (OverlayMat)
		{
			Mesh->SetOverlayMaterial(OverlayMat);
		}
	}

	UpdateReveal(0.f);
}

void ACaveboundPlaceBeamVFX::UpdateReveal(float Progress)
{
	const float T = FMath::Clamp(Progress, 0.f, 1.f);
	const float Ease = FMath::InterpEaseOut(0.f, 1.f, T, 2.f);

	if (OverlayMat)
	{
		FLinearColor Tint = BeamColor;
		if (bFadeIn)
		{
			// Build: strong hologram that eases toward the real mesh.
			Tint.A = FMath::Lerp(0.95f, 0.15f, Ease);
		}
		else
		{
			// Upgrade: solid beam color, no fade.
			Tint.A = 0.8f;
		}
		OverlayMat->SetVectorParameterValue(TEXT("Color"), Tint);
	}

	// Upgrades skip the scale pop-in.
	if (!bFadeIn)
	{
		return;
	}

	for (int32 i = 0; i < Meshes.Num(); ++i)
	{
		UStaticMeshComponent* Mesh = Meshes[i].Get();
		if (!IsValid(Mesh))
		{
			continue;
		}

		const FVector Base = MeshScales.IsValidIndex(i) ? MeshScales[i] : FVector::OneVector;
		Mesh->SetRelativeScale3D(Base * FMath::Lerp(0.2f, 1.f, Ease));
	}
}

void ACaveboundPlaceBeamVFX::ClearReveal()
{
	for (int32 i = 0; i < Meshes.Num(); ++i)
	{
		UStaticMeshComponent* Mesh = Meshes[i].Get();
		if (!IsValid(Mesh))
		{
			continue;
		}

		Mesh->SetOverlayMaterial(nullptr);
		if (MeshScales.IsValidIndex(i))
		{
			Mesh->SetRelativeScale3D(MeshScales[i]);
		}
	}

	if (ACaveboundTurret* Turret = Cast<ACaveboundTurret>(RevealActor.Get()))
	{
		Turret->SetRevealLocked(false);
	}

	Meshes.Reset();
	MeshScales.Reset();
	OverlayMat = nullptr;
}

void ACaveboundPlaceBeamVFX::StartPop()
{
	bPopping = true;
	PopTime = 0.f;

	if (BeamNiagara)
	{
		BeamNiagara->Deactivate();
	}
	if (BeamMesh)
	{
		BeamMesh->SetVisibility(false);
	}

	// Defender is fully shown before the pop flash.
	ClearReveal();
	if (OnDone)
	{
		TFunction<void()> Callback = MoveTemp(OnDone);
		OnDone = nullptr;
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

void ACaveboundPlaceBeamVFX::Finish()
{
	if (bDone)
	{
		return;
	}
	bDone = true;

	if (BeamNiagara)
	{
		BeamNiagara->Deactivate();
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

	// --- Pop flash phase ---
	if (bPopping)
	{
		PopTime += DeltaTime;
		const float T = FMath::Clamp(PopTime / FMath::Max(0.05f, PopDuration), 0.f, 1.f);
		const float Scale = FMath::InterpEaseOut(0.08f, 2.4f, T, 2.f);
		const float Alpha = 1.f - T;

		if (PopMesh)
		{
			PopMesh->SetRelativeScale3D(FVector(Scale));
			PopMesh->SetVisibility(Alpha > 0.02f);
		}
		if (PopMat)
		{
			const FLinearColor Lit = BeamColor * (0.5f + 2.2f * Alpha);
			PopMat->SetVectorParameterValue(TEXT("Color"), Lit);
			PopMat->SetVectorParameterValue(TEXT("EmissiveColor"), Lit);
			PopMat->SetScalarParameterValue(TEXT("Opacity"), Alpha);
			PopMat->SetScalarParameterValue(TEXT("Multiply"), Alpha);
		}

		if (T >= 1.f)
		{
			Finish();
		}
		return;
	}

	// --- Main beam phase ---
	Time += DeltaTime;
	const float T = FMath::Clamp(Time / FMath::Max(0.05f, Duration), 0.f, 1.f);

	// Rise quickly, then fade out near the end.
	const float Rise = FMath::InterpEaseOut(0.05f, 1.f, FMath::Clamp(T / 0.22f, 0.f, 1.f), 2.f);
	const float Fade = T < 0.55f ? 1.f : FMath::InterpEaseIn(1.f, 0.f, (T - 0.55f) / 0.45f, 2.f);
	const float Alpha = Rise * Fade;

	const float Height = (FMath::Max(10.f, BeamHeight) / 100.f) * Rise;
	const float Radius = (FMath::Max(1.f, BeamRadius) / 50.f) * FMath::Lerp(1.2f, 0.65f, T);
	BeamMesh->SetRelativeLocation(FVector(0.f, 0.f, BeamHeight * 0.5f * Rise));
	BeamMesh->SetRelativeScale3D(FVector(Radius, Radius, Height));
	BeamMesh->SetVisibility(Alpha > 0.02f);

	if (BeamMat)
	{
		const FLinearColor Lit = BeamColor * (0.35f + 1.8f * Alpha);
		BeamMat->SetVectorParameterValue(TEXT("Color"), Lit);
		BeamMat->SetVectorParameterValue(TEXT("EmissiveColor"), Lit);
		BeamMat->SetScalarParameterValue(TEXT("Opacity"), Alpha);
		BeamMat->SetScalarParameterValue(TEXT("Multiply"), Alpha);
	}

	// Defender finishes fading in before the pop.
	UpdateReveal(FMath::Clamp(T / 0.85f, 0.f, 1.f));

	if (T >= 1.f)
	{
		StartPop();
	}
}
