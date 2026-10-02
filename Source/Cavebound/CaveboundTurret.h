#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CaveboundDamageFlash.h"
#include "CaveboundHoverHealth.h"
#include "CaveboundTurret.generated.h"

class UArrowComponent;
class USceneComponent;
class UStaticMeshComponent;
class ACaveboundBaseEnemy;

// Generic turret
UCLASS(Blueprintable)
class CAVEBOUND_API ACaveboundTurret : public AActor, public ICaveboundHoverHealth
{
	GENERATED_BODY()

public:
	ACaveboundTurret();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// HUD popup bar on mouse hover
	virtual float GetHoverHealth_Implementation() const override { return Health; }
	virtual float GetHoverMaxHealth_Implementation() const override { return MaxHealth; }
	virtual FText GetHoverDisplayName_Implementation() const override { return FText::FromString(TEXT("Turret")); }

	UFUNCTION(BlueprintCallable, Category = "Cavebound")
	void ApplyDamage(float Amount);

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	int32 GetCost() const { return Cost; }

	static int32 GetCostForClass(TSubclassOf<AActor> DefenderClass);

	bool IsDestroyed() const { return Health <= 0.f; }

protected:
	virtual void PlayDamageFlash();
	virtual void OnDestroyedByDamage();

	ACaveboundBaseEnemy* FindNearestEnemy() const;
	void RememberFirePoint();
	void GetShotStart(FVector& shotLocation, FVector& shotForward) const;
	float GetYawErrorToEnemy(const ACaveboundBaseEnemy* enemy) const;
	void TurnTowardEnemy(const ACaveboundBaseEnemy* enemy, float DeltaTime);
	bool TryFireAtEnemy(ACaveboundBaseEnemy* closestEnemy);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turret")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(EditAnywhere, Category = "Turret", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float DefenderMeshScale = 0.3f;

	// Wood spent to place this turret
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Turret")
	int32 Cost = 40;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MaxHealth = 150.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	float Health = 150.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	bool bAutoFire = true;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AttackRange = 1200.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float FireInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (MakeEditWidget = true))
	FVector FirePointOffset = FVector(40.f, 0.f, 220.f);

	UPROPERTY(EditAnywhere, Category = "Combat")
	float TurnSpeed = 720.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float AimHalfAngle = 45.f;

	TObjectPtr<UArrowComponent> FirePoint;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ProjectileDamage = 10.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float ProjectileSpeed = 1200.f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TSubclassOf<AActor> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Combat")
	float HitFlashDuration = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Combat")
	FLinearColor HitFlashColor = FLinearColor(1.f, 0.f, 0.f);

	FTimerHandle HitFlashTimer;

	float FireTime = 0.f;
};
