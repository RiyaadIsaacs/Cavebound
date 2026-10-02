#include "CaveboundPoisonDome.h"

#include "CaveboundBaseEnemy.h"
#include "CaveboundGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Green sphere visual
	UMaterialInterface* MakeGreenDomeMaterial()
	{
		static TWeakObjectPtr<UMaterial> SavedMaterial;
		if (UMaterial* Existing = SavedMaterial.Get())
		{
			return Existing;
		}

#if WITH_EDITORONLY_DATA
		UMaterial* Green = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient);
		Green->MaterialDomain = MD_Surface;
		Green->BlendMode = BLEND_Translucent;
		Green->SetShadingModel(MSM_Unlit);
		Green->TwoSided = false;
		Green->TranslucencyLightingMode = TLM_Surface;

		Green->bAutomaticallySetUsageInEditor = true;

		if (UMaterialEditorOnlyData* EditorData = Green->GetEditorOnlyData())
		{
			UMaterialExpressionConstant3Vector* GreenColor = NewObject<UMaterialExpressionConstant3Vector>(Green);
			GreenColor->Constant = FLinearColor(0.1f, 0.85f, 0.15f, 1.f);
			EditorData->ExpressionCollection.AddExpression(GreenColor);
			GreenColor->ConnectExpression(&EditorData->EmissiveColor, 0);

			UMaterialExpressionConstant* SeeThroughAmount = NewObject<UMaterialExpressionConstant>(Green);
			SeeThroughAmount->R = 0.28f;
			EditorData->ExpressionCollection.AddExpression(SeeThroughAmount);
			SeeThroughAmount->ConnectExpression(&EditorData->Opacity, 0);
		}

		bool bNeedsRecompile = false;
		Green->SetMaterialUsage(bNeedsRecompile, MATUSAGE_StaticMesh);
		Green->PreEditChange(nullptr);
		Green->PostEditChange();
		SavedMaterial = Green;
		return Green;
#else
		return nullptr;
#endif
	}
}

ACaveboundPoisonDome::ACaveboundPoisonDome()
{
	bAutoFire = false;

	// Poison tower asset
	VisualMesh->SetRelativeScale3D(FVector(0.2f));
	VisualMesh->SetRelativeRotation(FRotator::ZeroRotator);
	VisualMesh->SetRelativeLocation(FVector::ZeroVector);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> towerMesh(TEXT("/Game/Assets/Models/Towers/SM_PoisonTower_LVL4.SM_PoisonTower_LVL4"));
	if (towerMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(towerMesh.Object);
	}

	// Transparent dome
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
	Super::BeginPlay();
	UpdatePoisonCloud();
}

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

// Updates cloud size
void ACaveboundPoisonDome::UpdatePoisonCloud()
{
	if (!PoisonCloud)
	{
		return;
	}

	PoisonCloud->bDisallowNanite = true;

	if (UMaterialInterface* Green = MakeGreenDomeMaterial())
	{
		PoisonCloud->SetMaterial(0, Green);
	}

	const float widthScale = DomeRadius / 50.f;
	PoisonCloud->SetRelativeScale3D(FVector(widthScale, widthScale, widthScale));
	PoisonCloud->SetRelativeLocation(FVector::ZeroVector);
}

// Enemy effects
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

	for (auto savedEnemy = PoisonTimeRemaining.CreateIterator(); savedEnemy; ++savedEnemy)
	{
		if (!enemiesInsideDome.Contains(savedEnemy.Key()))
		{
			savedEnemy.RemoveCurrent();
		}
	}
}
