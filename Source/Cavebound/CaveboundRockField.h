#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CaveboundRockField.generated.h"

/**
 * Sits on the procedural terrain and places rock piles on ground that is not a path.
 */
UCLASS(ClassGroup = (Cavebound), meta = (BlueprintSpawnableComponent))
class CAVEBOUND_API UCaveboundRockField : public UActorComponent
{
	GENERATED_BODY()

public:
	UCaveboundRockField();

	virtual void BeginPlay() override;

protected:
	void SpawnRocks();

	UPROPERTY(EditAnywhere, Category = "Rocks")
	int32 MaxRocks = 8;

	UPROPERTY(EditAnywhere, Category = "Rocks")
	float MinSpacing = 600.f;

	bool bSpawned = false;
	int32 SpawnTries = 0;
};
