#include "CaveboundEnemyBolt.h"
#include "CaveboundTurret.h"
#include "CaveboundTree.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundEnemyBolt::ACaveboundEnemyBolt()
{
	PrimaryActorTick.bCanEverTick = true;

	// A small ball so the mage's attack is visible.
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	SetRootComponent(VisualMesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> sphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (sphereMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(sphereMesh.Object);
	}

	VisualMesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.35f));
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACaveboundEnemyBolt::Init(AActor* InTarget, float InDamage)
{
	Target = InTarget;
	Damage = InDamage;
}

void ACaveboundEnemyBolt::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Age += DeltaTime;
	if (Age > LifeTime || !IsValid(Target))
	{
		Destroy();
		return;
	}

	// Fly toward the target.
	FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.Size() <= Speed * DeltaTime + 40.f)
	{
		HitTarget();
		return;
	}

	const FVector Step = ToTarget.GetSafeNormal() * Speed * DeltaTime;
	AddActorWorldOffset(Step);
}

void ACaveboundEnemyBolt::HitTarget()
{
	if (ACaveboundTurret* Turret = Cast<ACaveboundTurret>(Target))
	{
		if (!Turret->IsDestroyed())
		{
			Turret->ApplyDamage(Damage);
		}
	}
	else if (ACaveboundTree* TreeTarget = Cast<ACaveboundTree>(Target))
	{
		if (!TreeTarget->IsDestroyed())
		{
			TreeTarget->ApplyDamage(Damage);
		}
	}

	Destroy();
}
