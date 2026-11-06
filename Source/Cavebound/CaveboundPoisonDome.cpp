#include "CaveboundPoisonDome.h"

#include "CaveboundBaseEnemy.h"
#include "CaveboundGameMode.h"
#include "CaveboundShaderMaterials.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ACaveboundPoisonDome::ACaveboundPoisonDome()
{
	bAutoFire = false;

	// poison tower mesh — pivot already on the ground
	VisualMesh->SetRelativeScale3D(FVector(DefenderMeshScale));
	VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
	VisualMesh->SetRelativeLocation(FVector::ZeroVector);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> level1Mesh(TEXT("/Game/Assets/Models/Towers/SM_PoisonTower_LVL1.SM_PoisonTower_LVL1"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> level2Mesh(TEXT("/Game/Assets/Models/Towers/SM_PoisonTower_LVL2.SM_PoisonTower_LVL2"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> level3Mesh(TEXT("/Game/Assets/Models/Towers/SM_PoisonTower_LVL3.SM_PoisonTower_LVL3"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> fallbackMesh(TEXT("/Game/Assets/Models/Towers/SM_PoisonTower_LVL4.SM_PoisonTower_LVL4"));
	UseTowerMeshes(
		level1Mesh.Succeeded() ? level1Mesh.Object : nullptr,
		level2Mesh.Succeeded() ? level2Mesh.Object : nullptr,
		level3Mesh.Succeeded() ? level3Mesh.Object : nullptr,
		fallbackMesh.Succeeded() ? fallbackMesh.Object : nullptr);

	// transparent poison cloud sphere
	PoisonCloud = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PoisonCloud"));
	PoisonCloud->SetupAttachment(SceneRoot);
	PoisonCloud->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PoisonCloud->SetCastShadow(false);
	PoisonCloud->bDisallowNanite = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> sphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (sphereMesh.Succeeded())
	{
		PoisonCloud->SetStaticMesh(sphereMesh.Object);
	}

	Cost = 150;
	MaxHealth = 130.f;
	Health = MaxHealth;
}

void ACaveboundPoisonDome::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UpdatePoisonCloud();
}

void ACaveboundPoisonDome::BeginPlay()
{
	BasePoisonDamage = PoisonDamagePerSecond;
	Super::BeginPlay();
	UpdatePoisonCloud();
}

void ACaveboundPoisonDome::ApplyLevelBonus()
{
	Super::ApplyLevelBonus();
	// scales poison dps with upgrade level
	PoisonDamagePerSecond = BasePoisonDamage * (1.f + UpgradeLevel * 0.4f);
}

// creates and plays hit flash on the tower mesh only
void ACaveboundPoisonDome::PlayDamageFlash()
{
	TArray<UStaticMeshComponent*> tower;
	if (VisualMesh)
	{
		tower.Add(VisualMesh);
	}
	FCaveboundDamageFlash::FlashMeshes(this, tower, HitFlashTimer, FLinearColor::White, HitFlashDuration);
}

void ACaveboundPoisonDome::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsDestroyed())
	{
		return;
	}

	if (!IsCombatRound())
	{
		return;
	}

	UpdateDome(DeltaTime);
}

bool ACaveboundPoisonDome::IsCombatRound() const
{
	const UWorld* world = GetWorld();
	if (!world)
	{
		return false;
	}

	const ACaveboundGameMode* gameMode = world->GetAuthGameMode<ACaveboundGameMode>();
	return !gameMode || gameMode->GetRoundState() == ECaveboundRoundState::Combat;
}

// updates cloud size and assigns the pulsing poison shader
void ACaveboundPoisonDome::UpdatePoisonCloud()
{
	if (!PoisonCloud)
	{
		return;
	}

	PoisonCloud->bDisallowNanite = true;

	if (!GreenMaterial)
	{
		GreenMaterial = CaveboundShaderMaterials::CreatePoisonPulseMID(this);
	}

	if (GreenMaterial)
	{
		PoisonCloud->SetMaterial(0, GreenMaterial);
	}

	const float widthScale = DomeRadius / 50.f;
	PoisonCloud->SetRelativeScale3D(FVector(widthScale, widthScale, widthScale));
	PoisonCloud->SetRelativeLocation(FVector::ZeroVector);
}

// slows and ticks poison damage on enemies inside the dome
void ACaveboundPoisonDome::UpdateDome(float DeltaTime)
{
	UWorld* world = GetWorld();
	if (!world)
	{
		return;
	}

	TArray<AActor*> allEnemies;
	UGameplayStatics::GetAllActorsOfClass(world, ACaveboundBaseEnemy::StaticClass(), allEnemies);

	TArray<TWeakObjectPtr<ACaveboundBaseEnemy>> enemiesInsideDome;
	const FVector domeLocation = GetActorLocation();

	for (AActor* foundActor : allEnemies)
	{
		ACaveboundBaseEnemy* enemy = Cast<ACaveboundBaseEnemy>(foundActor);
		if (!enemy || enemy->IsDead())
		{
			continue;
		}

		if (FVector::Dist2D(domeLocation, enemy->GetActorLocation()) > DomeRadius)
		{
			continue;
		}

		enemy->ApplySlow(SlowMultiplier);
		enemiesInsideDome.Add(enemy);

		float& timeUntilNextPoisonHit = PoisonTimeRemaining.FindOrAdd(enemy, 0.f);
		timeUntilNextPoisonHit -= DeltaTime;
		if (timeUntilNextPoisonHit <= 0.f)
		{
			enemy->ApplyDamage(PoisonDamagePerSecond * PoisonTickInterval);
			timeUntilNextPoisonHit = PoisonTickInterval;
		}
	}

	// drops timers for enemies that left the dome
	for (auto savedEnemy = PoisonTimeRemaining.CreateIterator(); savedEnemy; ++savedEnemy)
	{
		if (!enemiesInsideDome.Contains(savedEnemy.Key()))
		{
			savedEnemy.RemoveCurrent();
		}
	}
}
