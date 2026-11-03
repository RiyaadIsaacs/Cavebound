#include "CaveboundSoftVFX.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

// Shared helpers for the soft sphere VFX.
namespace SoftVFX
{
	UStaticMesh* SphereMesh()
	{
		static TWeakObjectPtr<UStaticMesh> Cached;
		if (UStaticMesh* Mesh = Cached.Get())
		{
			return Mesh;
		}

		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		Cached = Mesh;
		return Mesh;
	}

	UMaterialInterface* SoftMat()
	{
		return LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	}

	/** Bright emissive material — makes embers actually glow. */
	UMaterialInterface* GlowMat()
	{
		return LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	}

	/** Makes a small non-colliding sphere and optional dynamic material. */
	UStaticMeshComponent* MakeBall(
		AActor* Owner,
		USceneComponent* Parent,
		UStaticMesh* Mesh,
		UMaterialInterface* BaseMat,
		const FName Name,
		UMaterialInstanceDynamic*& OutMat)
	{
		UStaticMeshComponent* Ball = NewObject<UStaticMeshComponent>(Owner, Name);
		Ball->SetupAttachment(Parent);
		Ball->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Ball->SetCastShadow(false);
		Ball->SetReceivesDecals(false);
		Ball->SetStaticMesh(Mesh);
		Ball->RegisterComponent();
		if (BaseMat)
		{
			OutMat = Ball->CreateDynamicMaterialInstance(0, BaseMat);
		}
		return Ball;
	}
}

// ---------------------------------------------------------------------------
// Hit sparks
// ---------------------------------------------------------------------------

ACaveboundSparkBurst::ACaveboundSparkBurst()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	SetActorEnableCollision(false);
}

void ACaveboundSparkBurst::SpawnAt(UWorld* World, const FVector& Location, const FLinearColor& InColor)
{
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ACaveboundSparkBurst* Burst = World->SpawnActor<ACaveboundSparkBurst>(
			ACaveboundSparkBurst::StaticClass(),
			Location,
			FRotator::ZeroRotator,
			Params))
	{
		Burst->Color = InColor;
	}
}

void ACaveboundSparkBurst::BeginPlay()
{
	Super::BeginPlay();

	UStaticMesh* Mesh = SoftVFX::SphereMesh();
	UMaterialInterface* BaseMat = SoftVFX::SoftMat();
	if (!Mesh)
	{
		Destroy();
		return;
	}

	Balls.Reset();
	Mats.Reset();
	Speeds.Reset();

	for (int32 i = 0; i < Count; ++i)
	{
		UMaterialInstanceDynamic* Mat = nullptr;
		UStaticMeshComponent* Ball = SoftVFX::MakeBall(
			this,
			Root,
			Mesh,
			BaseMat,
			*FString::Printf(TEXT("Spark_%d"), i),
			Mat);

		const float Size = FMath::FRandRange(0.04f, 0.09f);
		Ball->SetRelativeScale3D(FVector(Size));
		Ball->SetRelativeLocation(FVector(
			FMath::FRandRange(-12.f, 12.f),
			FMath::FRandRange(-12.f, 12.f),
			FMath::FRandRange(10.f, 40.f)));

		if (Mat)
		{
			FLinearColor Lit = Color * 2.2f;
			Lit.A = 1.f;
			Mat->SetVectorParameterValue(TEXT("Color"), Lit);
		}

		// Shoot outward + a bit upward.
		const FVector Dir = FMath::VRand().GetSafeNormal();
		Speeds.Add(Dir * FMath::FRandRange(220.f, 520.f) + FVector(0.f, 0.f, FMath::FRandRange(80.f, 220.f)));
		Balls.Add(Ball);
		Mats.Add(Mat);
	}
}

void ACaveboundSparkBurst::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Time += DeltaTime;
	const float T = FMath::Clamp(Time / FMath::Max(0.05f, Duration), 0.f, 1.f);
	const float Fade = 1.f - T;

	for (int32 i = 0; i < Balls.Num(); ++i)
	{
		UStaticMeshComponent* Ball = Balls[i];
		if (!Ball)
		{
			continue;
		}

		// Gravity + move.
		Speeds[i] += FVector(0.f, 0.f, -650.f) * DeltaTime;
		Ball->AddRelativeLocation(Speeds[i] * DeltaTime);
		Ball->SetRelativeScale3D(FVector(FMath::Lerp(0.08f, 0.02f, T)));

		if (UMaterialInstanceDynamic* Mat = Mats.IsValidIndex(i) ? Mats[i] : nullptr)
		{
			FLinearColor Lit = Color * (0.6f + 1.8f * Fade);
			Lit.A = Fade;
			Mat->SetVectorParameterValue(TEXT("Color"), Lit);
			Mat->SetScalarParameterValue(TEXT("Opacity"), Fade);
		}
	}

	if (T >= 1.f)
	{
		Destroy();
	}
}

// ---------------------------------------------------------------------------
// Hovering embers
// ---------------------------------------------------------------------------

ACaveboundHoverEmbers::ACaveboundHoverEmbers()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	SetActorEnableCollision(false);
}

ACaveboundHoverEmbers* ACaveboundHoverEmbers::SpawnAttached(
	AActor* Parent,
	const FVector& LocalOffset,
	const FLinearColor& InColor)
{
	if (!Parent || !Parent->GetWorld())
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.Owner = Parent;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACaveboundHoverEmbers* Embers = Parent->GetWorld()->SpawnActor<ACaveboundHoverEmbers>(
		ACaveboundHoverEmbers::StaticClass(),
		Parent->GetActorLocation(),
		FRotator::ZeroRotator,
		Params);
	if (!Embers)
	{
		return nullptr;
	}

	Embers->Color = InColor;
	Embers->AttachToActor(Parent, FAttachmentTransformRules::KeepWorldTransform);
	Embers->SetActorRelativeLocation(LocalOffset);
	return Embers;
}

void ACaveboundHoverEmbers::BeginPlay()
{
	Super::BeginPlay();

	UStaticMesh* Mesh = SoftVFX::SphereMesh();
	// Prefer emissive so they glow; fall back to soft translucent.
	UMaterialInterface* BaseMat = SoftVFX::GlowMat();
	if (!BaseMat)
	{
		BaseMat = SoftVFX::SoftMat();
	}
	if (!Mesh)
	{
		return;
	}

	Balls.Reset();
	Mats.Reset();
	Phase.Reset();
	RadiusScale.Reset();

	for (int32 i = 0; i < Count; ++i)
	{
		UMaterialInstanceDynamic* Mat = nullptr;
		UStaticMeshComponent* Ball = SoftVFX::MakeBall(
			this,
			Root,
			Mesh,
			BaseMat,
			*FString::Printf(TEXT("Ember_%d"), i),
			Mat);

		Ball->SetRelativeScale3D(FVector(FMath::FRandRange(0.05f, 0.1f)));
		if (Mat)
		{
			const FLinearColor Lit = Color * 3.5f;
			Mat->SetVectorParameterValue(TEXT("Color"), Lit);
			Mat->SetVectorParameterValue(TEXT("EmissiveColor"), Lit);
			Mat->SetScalarParameterValue(TEXT("Multiply"), 2.5f);
		}

		Balls.Add(Ball);
		Mats.Add(Mat);
		Phase.Add(FMath::FRandRange(0.f, PI * 2.f));
		RadiusScale.Add(FMath::FRandRange(0.65f, 1.25f));
	}
}

void ACaveboundHoverEmbers::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Time += DeltaTime;

	for (int32 i = 0; i < Balls.Num(); ++i)
	{
		UStaticMeshComponent* Ball = Balls[i];
		if (!Ball)
		{
			continue;
		}

		const float P = Phase[i];
		const float Angle = Time * (0.45f + 0.15f * i) + P;
		const float R = Radius * RadiusScale[i];
		const float Bob = FMath::Sin(Time * 1.7f + P) * 35.f;
		const float Z = Height * 0.5f + FMath::Sin(Time * 0.9f + P * 1.3f) * Height * 0.45f;

		Ball->SetRelativeLocation(FVector(
			FMath::Cos(Angle) * R,
			FMath::Sin(Angle * 0.85f) * R,
			Z + Bob));

		if (UMaterialInstanceDynamic* Mat = Mats.IsValidIndex(i) ? Mats[i] : nullptr)
		{
			// Pulse the emissive brightness — hue stays deep warm green.
			const float Pulse = 0.65f + 0.35f * (0.5f + 0.5f * FMath::Sin(Time * 3.f + P));
			const FLinearColor Lit = Color * (2.5f + 3.5f * Pulse);
			Mat->SetVectorParameterValue(TEXT("Color"), Lit);
			Mat->SetVectorParameterValue(TEXT("EmissiveColor"), Lit);
			Mat->SetScalarParameterValue(TEXT("Multiply"), 1.5f + 2.5f * Pulse);
			Mat->SetScalarParameterValue(TEXT("Opacity"), 0.85f + 0.15f * Pulse);
		}
	}
}
