#include "CaveboundTreeProjectile.h"
#include "CaveboundBaseEnemy.h"
#include "CaveboundTree.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundTreeProjectile::ACaveboundTreeProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetRelativeScale3D(FVector(0.35f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mesh->SetGenerateOverlapEvents(true);
	Mesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded())
	{
		Mesh->SetStaticMesh(Sphere.Object);
	}
}

void ACaveboundTreeProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (!Mesh)
	{
		return;
	}

	Mesh->OnComponentBeginOverlap.AddDynamic(this, &ACaveboundTreeProjectile::OnOverlap);

	// Soft green glow material.
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	if (Base)
	{
		Mat = Mesh->CreateDynamicMaterialInstance(0, Base);
		if (Mat)
		{
			Mat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.2f, 1.f, 0.35f, 0.95f));
		}
	}
}

void ACaveboundTreeProjectile::Init(ACaveboundBaseEnemy* InTarget, float InDamage, float InSpeed)
{
	Target = InTarget;
	Damage = InDamage;
	Speed = InSpeed;
	Time = 0.f;
}

void ACaveboundTreeProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Time += DeltaTime;
	if (Time >= MaxLife)
	{
		Destroy();
		return;
	}

	// Soft pulse + spin so it reads as magic, not a rock.
	const float Size = 0.3f + 0.08f * FMath::Sin(Time * 10.f);
	Mesh->SetRelativeScale3D(FVector(Size));
	AddActorWorldRotation(FRotator(0.f, 240.f * DeltaTime, 120.f * DeltaTime));

	if (Mat)
	{
		const float Glow = 0.75f + 0.35f * (0.5f + 0.5f * FMath::Sin(Time * 12.f));
		FLinearColor Lit = FLinearColor(0.15f, 1.f, 0.4f, 0.95f) * (1.1f + Glow);
		Lit.A = 0.75f + 0.2f * Glow;
		Mat->SetVectorParameterValue(TEXT("Color"), Lit);
		Mat->SetScalarParameterValue(TEXT("Opacity"), Lit.A);
	}

	ACaveboundBaseEnemy* Enemy = Target.Get();
	if (!Enemy || Enemy->IsDead())
	{
		Destroy();
		return;
	}

	const FVector ToEnemy = Enemy->GetActorLocation() - GetActorLocation();
	if (ToEnemy.Size() <= 40.f)
	{
		HitEnemy(Enemy);
		return;
	}

	AddActorWorldOffset(ToEnemy.GetSafeNormal() * Speed * DeltaTime);
}

void ACaveboundTreeProjectile::OnOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	HitEnemy(OtherActor);
}

void ACaveboundTreeProjectile::HitEnemy(AActor* OtherActor)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	// Don't hit the tree or other orbs.
	if (Cast<ACaveboundTree>(OtherActor) || Cast<ACaveboundTreeProjectile>(OtherActor))
	{
		return;
	}

	ACaveboundBaseEnemy* Enemy = Cast<ACaveboundBaseEnemy>(OtherActor);
	if (!Enemy || Enemy->IsDead())
	{
		return;
	}

	Enemy->ApplyDamage(Damage);
	Destroy();
}
