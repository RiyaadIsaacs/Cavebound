#include "CaveboundArrow.h"
#include "CaveboundBaseEnemy.h"
#include "CaveboundTurret.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundArrow::ACaveboundArrow()
{
	PrimaryActorTick.bCanEverTick = true;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	SetRootComponent(VisualMesh);
	VisualMesh->SetRelativeScale3D(FVector(0.15f, 0.15f, 0.6f));
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	VisualMesh->SetCollisionObjectType(ECC_WorldDynamic);
	VisualMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	// Enemies are WorldDynamic — Overlap+Overlap (see enemy tweak) generates BeginOverlap
	VisualMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	VisualMesh->SetGenerateOverlapEvents(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CylinderMesh.Object);
	}
}

void ACaveboundArrow::BeginPlay()
{
	Super::BeginPlay();

	if (VisualMesh)
	{
		VisualMesh->OnComponentBeginOverlap.AddDynamic(this, &ACaveboundArrow::OnMeshBeginOverlap);
	}

	if (AActor* ArrowOwner = GetOwner())
	{
		VisualMesh->MoveIgnoreActors.Add(ArrowOwner);
	}
}

void ACaveboundArrow::Init(ACaveboundBaseEnemy* InTarget, float InDamage, float InSpeed)
{
	Target = InTarget;
	Damage = InDamage;
	Speed = InSpeed;
	Lifetime = 0.f;
	bHasHit = false;
}

void ACaveboundArrow::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bHasHit)
	{
		return;
	}

	Lifetime += DeltaTime;
	if (Lifetime >= MaxLifetime)
	{
		Destroy();
		return;
	}

	ACaveboundBaseEnemy* CurrentTarget = Target.Get();
	if (!CurrentTarget || CurrentTarget->IsDead())
	{
		Destroy();
		return;
	}

	const FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	if (ToTarget.Size() <= HitRadius)
	{
		TryHitEnemy(CurrentTarget);
		return;
	}

	const FVector Step = ToTarget.GetSafeNormal() * Speed * DeltaTime;
	AddActorWorldOffset(Step, true);
	SetActorRotation(ToTarget.Rotation());
}

void ACaveboundArrow::OnMeshBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	TryHitEnemy(OtherActor);
}

void ACaveboundArrow::TryHitEnemy(AActor* OtherActor)
{
	if (bHasHit || !OtherActor || OtherActor == this || OtherActor == GetOwner())
	{
		return;
	}

	if (Cast<ACaveboundTurret>(OtherActor))
	{
		return;
	}

	ACaveboundBaseEnemy* Enemy = Cast<ACaveboundBaseEnemy>(OtherActor);
	if (!Enemy || Enemy->IsDead())
	{
		return;
	}

	bHasHit = true;
	Enemy->ApplyDamage(Damage);
	Destroy();
}
