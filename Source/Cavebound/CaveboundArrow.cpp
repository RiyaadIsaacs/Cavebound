#include "CaveboundArrow.h"
#include "CaveboundBaseEnemy.h"
#include "CaveboundTurret.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundArrow::ACaveboundArrow()
{
	PrimaryActorTick.bCanEverTick = true;

	// A small cylinder. It only overlaps things. It does not bump the world.
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	SetRootComponent(VisualMesh);
	VisualMesh->SetRelativeScale3D(FVector(0.15f, 0.15f, 0.6f));
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	VisualMesh->SetCollisionObjectType(ECC_WorldDynamic);
	VisualMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	// Enemies are also WorldDynamic, so both sides must overlap for the hit to count.
	VisualMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	VisualMesh->SetGenerateOverlapEvents(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> cylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (cylinderMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(cylinderMesh.Object);
	}
}

void ACaveboundArrow::BeginPlay()
{
	Super::BeginPlay();

	// Listen for the mesh touching an enemy.
	if (VisualMesh)
	{
		VisualMesh->OnComponentBeginOverlap.AddDynamic(this, &ACaveboundArrow::OnMeshBeginOverlap);
	}

	// Do not collide with the turret that fired this shot.
	if (AActor* turretThatFired = GetOwner())
	{
		VisualMesh->MoveIgnoreActors.Add(turretThatFired);
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

	// One hit is enough. Stop moving after that.
	if (bHasHit)
	{
		return;
	}

	// Delete the shot if it has been in the air too long.
	Lifetime += DeltaTime;
	if (Lifetime >= MaxLifetime)
	{
		Destroy();
		return;
	}

	// Delete the shot if the enemy is already gone.
	ACaveboundBaseEnemy* enemyToFollow = Target.Get();
	if (!enemyToFollow || enemyToFollow->IsDead())
	{
		Destroy();
		return;
	}

	const FVector directionToEnemy = enemyToFollow->GetActorLocation() - GetActorLocation();
	if (directionToEnemy.Size() <= HitRadius)
	{
		TryHitEnemy(enemyToFollow);
		return;
	}

	// Step toward the enemy and turn to face them.
	const FVector stepTowardEnemy = directionToEnemy.GetSafeNormal() * Speed * DeltaTime;
	AddActorWorldOffset(stepTowardEnemy, true);
	SetActorRotation(directionToEnemy.Rotation());
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
	// Ignore a second hit, ourselves, and the turret that shot us.
	if (bHasHit || !OtherActor || OtherActor == this || OtherActor == GetOwner())
	{
		return;
	}

	if (Cast<ACaveboundTurret>(OtherActor))
	{
		return;
	}

	ACaveboundBaseEnemy* enemy = Cast<ACaveboundBaseEnemy>(OtherActor);
	if (!enemy || enemy->IsDead())
	{
		return;
	}

	bHasHit = true;
	enemy->ApplyDamage(Damage);
	Destroy();
}
