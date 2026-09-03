#include "CaveboundTreeProjectile.h"
#include "CaveboundBaseEnemy.h"
#include "CaveboundTree.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundTreeProjectile::ACaveboundTreeProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	SetRootComponent(VisualMesh);
	VisualMesh->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.18f));
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	VisualMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	VisualMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	VisualMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	VisualMesh->SetGenerateOverlapEvents(true);

	// Load a basic cube mesh for projectile
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMesh.Object);
	}
}

void ACaveboundTreeProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (VisualMesh)
	{
		VisualMesh->OnComponentBeginOverlap.AddDynamic(this, &ACaveboundTreeProjectile::OnMeshBeginOverlap);
	}
}

void ACaveboundTreeProjectile::Init(ACaveboundBaseEnemy* InTarget, float InDamage, float InSpeed)
{
	Target = InTarget;
	Damage = InDamage;
	Speed = InSpeed;
	Lifetime = 0.f;
}

void ACaveboundTreeProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

	// projectile move towards the target
	const FVector ToTarget = CurrentTarget->GetActorLocation() - GetActorLocation();
	if (ToTarget.Size() <= 40.f)
	{
		TryHitEnemy(CurrentTarget);
		return;
	}

	AddActorWorldOffset(ToTarget.GetSafeNormal() * Speed * DeltaTime);
}

void ACaveboundTreeProjectile::OnMeshBeginOverlap(UPrimitiveComponent* OverlappedComponent,AActor* OtherActor,UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	TryHitEnemy(OtherActor);
}

void ACaveboundTreeProjectile::TryHitEnemy(AActor* OtherActor)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

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
