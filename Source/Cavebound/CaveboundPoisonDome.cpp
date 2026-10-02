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
	// A plain unlit green material. The engine sphere is Nanite, which cannot be see-through,
	// so this is only used after Nanite is turned off on the dome mesh.
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
	// The dome does not shoot. It slows and poisons enemies that walk into it.
	bAutoFire = false;

	// The engine cone points up. Flip it so the tip sits on the ground and the wide end is at the top.
	// Old body was about 160 tall. This one is 400 tall.
	const float bodyHeight = 400.f;
	VisualMesh->SetRelativeScale3D(FVector(1.2f, 1.2f, bodyHeight / 100.f));
	VisualMesh->SetRelativeRotation(FRotator(180.f, 0.f, 0.f));
	VisualMesh->SetRelativeLocation(FVector(0.f, 0.f, bodyHeight * 0.5f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> coneMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (coneMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(coneMesh.Object);
	}

	// See-through green dome. No collision, so enemies can walk through it.
	PoisonCloud = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PoisonCloud"));
	PoisonCloud->SetupAttachment(SceneRoot);
	PoisonCloud->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PoisonCloud->SetCastShadow(false);
	// Nanite ignores see-through materials and draws a solid white ball.
	PoisonCloud->bDisallowNanite = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> sphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (sphereMesh.Succeeded())
	{
		PoisonCloud->SetStaticMesh(sphereMesh.Object);
	}

	Cost = 190;
	MaxHealth = 130.f;
	Health = MaxHealth;
}

void ACaveboundPoisonDome::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Match the sphere to the poison range after blueprint values are applied.
	UpdatePoisonCloud();
}

void ACaveboundPoisonDome::BeginPlay()
{
	Super::BeginPlay();
	UpdatePoisonCloud();
}

void ACaveboundPoisonDome::PlayDamageFlash()
{
	// Only the cone flashes. The green dome stays as it is.
	TArray<UStaticMeshComponent*> coneOnly;
	if (VisualMesh)
	{
		coneOnly.Add(VisualMesh);
	}
	FCaveboundDamageFlash::FlashMeshes(this, coneOnly, HitFlashTimer, FLinearColor::White, HitFlashDuration);
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

	// If there is no game mode, still poison. Otherwise only during combat.
	const ACaveboundGameMode* gameMode = world->GetAuthGameMode<ACaveboundGameMode>();
	return !gameMode || gameMode->GetRoundState() == ECaveboundRoundState::Combat;
}

void ACaveboundPoisonDome::UpdatePoisonCloud()
{
	if (!PoisonCloud)
	{
		return;
	}

	// Nanite draws the solid white ball and ignores a see-through material.
	PoisonCloud->bDisallowNanite = true;

	if (UMaterialInterface* Green = MakeGreenDomeMaterial())
	{
		PoisonCloud->SetMaterial(0, Green);
	}

	// The engine sphere is centered on its origin and has a radius of 50.
	// Width matches the poison range. The center sits on the ground, so the top half is the dome.
	const float widthScale = DomeRadius / 50.f;
	PoisonCloud->SetRelativeScale3D(FVector(widthScale, widthScale, widthScale));
	PoisonCloud->SetRelativeLocation(FVector::ZeroVector);
}

void ACaveboundPoisonDome::UpdateDome(float DeltaTime)
{
	UWorld* world = GetWorld();
	if (!world)
	{
		return;
	}

	TArray<AActor*> allEnemies;
	UGameplayStatics::GetAllActorsOfClass(world, ACaveboundBaseEnemy::StaticClass(), allEnemies);

	// Enemies standing inside the dome this frame.
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

		// Slow them, then deal a poison hit on a timer.
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

	// Forget enemies that walked out, so the next time they enter the timer starts again.
	for (auto savedEnemy = PoisonTimeRemaining.CreateIterator(); savedEnemy; ++savedEnemy)
	{
		if (!enemiesInsideDome.Contains(savedEnemy.Key()))
		{
			savedEnemy.RemoveCurrent();
		}
	}
}
