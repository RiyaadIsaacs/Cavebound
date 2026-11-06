#include "CaveboundSoftVFX.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

// shared helpers for soft sphere VFX
namespace SoftVFX
{
	// caches the engine sphere mesh
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

	// translucent soft material
	UMaterialInterface* SoftMat()
	{
		return LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	}

	// bright emissive material for glowing sparks / embers
	UMaterialInterface* GlowMat()
	{
		return LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
	}

	// creates a small non-colliding sphere with optional MID
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

// creates and plays hit spark burst at a world location
void ACaveboundSparkBurst::SpawnAt(
	UWorld* World,
	const FVector& Location,
	const FLinearColor& InColor,
	int32 InCount,
	float InSizeScale)
{
	if (!World)
	{
		return;
	}

	const FTransform SpawnTM(FRotator::ZeroRotator, Location);
	ACaveboundSparkBurst* Burst = World->SpawnActorDeferred<ACaveboundSparkBurst>(
		ACaveboundSparkBurst::StaticClass(),
		SpawnTM,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Burst)
	{
		return;
	}

	Burst->Color = InColor;
	if (InCount > 0)
	{
		Burst->Count = InCount;
	}
	Burst->SizeScale = FMath::Max(0.1f, InSizeScale);
	Burst->FinishSpawning(SpawnTM);
}

void ACaveboundSparkBurst::BeginPlay()
{
	Super::BeginPlay();

	UStaticMesh* Mesh = SoftVFX::SphereMesh();
	// prefer emissive so grey/colored sparks stay readable
	UMaterialInterface* BaseMat = SoftVFX::GlowMat();
	if (!BaseMat)
	{
		BaseMat = SoftVFX::SoftMat();
	}
	if (!Mesh)
	{
		Destroy();
		return;
	}

	const float Scale = FMath::Max(0.1f, SizeScale);
	Balls.Reset();
	Mats.Reset();
	Speeds.Reset();

	// spawns each spark ball with random size / velocity
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

		const float Size = FMath::FRandRange(0.04f, 0.09f) * Scale;
		Ball->SetRelativeScale3D(FVector(Size));
		Ball->SetRelativeLocation(FVector(
			FMath::FRandRange(-12.f, 12.f) * Scale,
			FMath::FRandRange(-12.f, 12.f) * Scale,
			FMath::FRandRange(10.f, 40.f) * Scale));

		if (Mat)
		{
			FLinearColor Lit = Color * 2.2f;
			Lit.A = 1.f;
			Mat->SetVectorParameterValue(TEXT("Color"), Lit);
			Mat->SetVectorParameterValue(TEXT("EmissiveColor"), Lit);
		}

		// shoots outward + a bit upward
		const FVector Dir = FMath::VRand().GetSafeNormal();
		Speeds.Add(
			Dir * FMath::FRandRange(220.f, 520.f) * Scale
			+ FVector(0.f, 0.f, FMath::FRandRange(80.f, 220.f) * Scale));
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

	// moves / fades sparks, then destroys when done
	for (int32 i = 0; i < Balls.Num(); ++i)
	{
		UStaticMeshComponent* Ball = Balls[i];
		if (!Ball)
		{
			continue;
		}

		// gravity + move
		const float Scale = FMath::Max(0.1f, SizeScale);
		Speeds[i] += FVector(0.f, 0.f, -650.f) * DeltaTime;
		Ball->AddRelativeLocation(Speeds[i] * DeltaTime);
		Ball->SetRelativeScale3D(FVector(FMath::Lerp(0.08f, 0.02f, T) * Scale));

		if (UMaterialInstanceDynamic* Mat = Mats.IsValidIndex(i) ? Mats[i] : nullptr)
		{
			FLinearColor Lit = Color * (0.6f + 1.8f * Fade);
			Lit.A = Fade;
			Mat->SetVectorParameterValue(TEXT("Color"), Lit);
			Mat->SetVectorParameterValue(TEXT("EmissiveColor"), Lit);
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

// attaches looping ember orbs to a parent actor
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

	// orbits and pulses each ember around the parent
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
			// pulses emissive brightness
			const float Pulse = 0.65f + 0.35f * (0.5f + 0.5f * FMath::Sin(Time * 3.f + P));
			const FLinearColor Lit = Color * (2.5f + 3.5f * Pulse);
			Mat->SetVectorParameterValue(TEXT("Color"), Lit);
			Mat->SetVectorParameterValue(TEXT("EmissiveColor"), Lit);
			Mat->SetScalarParameterValue(TEXT("Multiply"), 1.5f + 2.5f * Pulse);
			Mat->SetScalarParameterValue(TEXT("Opacity"), 0.85f + 0.15f * Pulse);
		}
	}
}

// ---------------------------------------------------------------------------
// Magical spawn poof
// ---------------------------------------------------------------------------

void ACaveboundMagicPoof::SetSoftColor(UMaterialInstanceDynamic* Mat, const FLinearColor& Tint, float Alpha)
{
	if (!Mat)
	{
		return;
	}

	// Widget translucent mat only cares about Color.A for fade — keep RGB soft (no HDR blowout).
	FLinearColor Soft = Tint;
	Soft.R = FMath::Clamp(Soft.R, 0.f, 1.f);
	Soft.G = FMath::Clamp(Soft.G, 0.f, 1.f);
	Soft.B = FMath::Clamp(Soft.B, 0.f, 1.f);
	Soft.A = FMath::Clamp(Alpha, 0.f, 1.f);
	Mat->SetVectorParameterValue(TEXT("Color"), Soft);
}

ACaveboundMagicPoof::ACaveboundMagicPoof()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	SetActorEnableCollision(false);
}

// creates and plays spawn poof at a world location
void ACaveboundMagicPoof::SpawnAt(UWorld* World, const FVector& Location)
{
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<ACaveboundMagicPoof>(
		ACaveboundMagicPoof::StaticClass(),
		Location,
		FRotator::ZeroRotator,
		Params);
}

void ACaveboundMagicPoof::BeginPlay()
{
	Super::BeginPlay();

	UStaticMesh* Mesh = SoftVFX::SphereMesh();
	// soft translucent only — emissive looked broken on this effect
	UMaterialInterface* Soft = SoftVFX::SoftMat();
	if (!Mesh || !Soft)
	{
		Destroy();
		return;
	}

	// bright core puff
	{
		UMaterialInstanceDynamic* Mat = nullptr;
		Core = SoftVFX::MakeBall(this, Root, Mesh, Soft, TEXT("PoofCore"), Mat);
		Core->SetRelativeScale3D(FVector(0.12f));
		Core->SetTranslucentSortPriority(2);
		CoreMat = Mat;
		SetSoftColor(CoreMat, FMath::Lerp(Color, FLinearColor(0.9f, 0.85f, 1.f), 0.35f), 0.9f);
	}

	Puffs.Reset();
	PuffMats.Reset();
	Speeds.Reset();
	StartSizes.Reset();
	BaseTints.Reset();

	// outer soft puffs that drift and fade
	for (int32 i = 0; i < Count; ++i)
	{
		UMaterialInstanceDynamic* Mat = nullptr;
		UStaticMeshComponent* Puff = SoftVFX::MakeBall(
			this,
			Root,
			Mesh,
			Soft,
			*FString::Printf(TEXT("Poof_%d"), i),
			Mat);

		const float Size = FMath::FRandRange(0.1f, 0.26f);
		Puff->SetRelativeScale3D(FVector(Size * 0.2f));
		Puff->SetRelativeLocation(FVector(
			FMath::FRandRange(-16.f, 16.f),
			FMath::FRandRange(-16.f, 16.f),
			FMath::FRandRange(8.f, 48.f)));
		Puff->SetTranslucentSortPriority(1);

		const float Mix = FMath::FRandRange(0.f, 1.f);
		FLinearColor Tint = FMath::Lerp(Color, FLinearColor(0.8f, 0.88f, 1.f), Mix * 0.4f);
		SetSoftColor(Mat, Tint, 0.75f);

		FVector Dir = FMath::VRand();
		Dir.Z = FMath::Abs(Dir.Z) * 0.5f + 0.5f;
		Dir.Normalize();
		Speeds.Add(Dir * FMath::FRandRange(140.f, 300.f));
		StartSizes.Add(Size);
		BaseTints.Add(Tint);
		Puffs.Add(Puff);
		PuffMats.Add(Mat);
	}
}

void ACaveboundMagicPoof::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Time += DeltaTime;
	const float T = FMath::Clamp(Time / FMath::Max(0.05f, Duration), 0.f, 1.f);

	// expands quickly, then eases out to fully transparent
	const float Expand = FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(T / 0.35f, 0.f, 1.f), 2.f);
	const float Fade = FMath::InterpEaseIn(1.f, 0.f, T, 2.2f);

	if (Core)
	{
		const float CoreSize = FMath::Lerp(0.12f, 0.85f, Expand) * FMath::Lerp(1.f, 0.35f, T);
		Core->SetRelativeScale3D(FVector(CoreSize));
		Core->SetVisibility(Fade > 0.02f);
		SetSoftColor(
			CoreMat,
			FMath::Lerp(Color, FLinearColor(0.9f, 0.85f, 1.f), 0.35f),
			Fade * 0.85f);
	}

	for (int32 i = 0; i < Puffs.Num(); ++i)
	{
		UStaticMeshComponent* Puff = Puffs[i];
		if (!Puff)
		{
			continue;
		}

		Speeds[i] *= FMath::Clamp(1.f - DeltaTime * 2.2f, 0.65f, 1.f);
		Speeds[i].Z += 25.f * DeltaTime;
		Puff->AddRelativeLocation(Speeds[i] * DeltaTime);

		// Grow, then shrink away as they fade so nothing pops off.
		const float Grow = FMath::Lerp(StartSizes[i] * 0.2f, StartSizes[i] * 1.35f, Expand);
		const float Size = Grow * FMath::Lerp(1.f, 0.45f, T);
		Puff->SetRelativeScale3D(FVector(Size));
		Puff->SetVisibility(Fade > 0.02f);

		const FLinearColor Tint = BaseTints.IsValidIndex(i) ? BaseTints[i] : Color;
		SetSoftColor(PuffMats.IsValidIndex(i) ? PuffMats[i] : nullptr, Tint, Fade * 0.7f);
	}

	if (T >= 1.f || Fade <= 0.01f)
	{
		Destroy();
	}
}
