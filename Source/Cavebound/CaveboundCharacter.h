#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CaveboundCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class ACaveboundTree;

/**
  The player's body in the world
 */
UCLASS()
class CAVEBOUND_API ACaveboundCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACaveboundCharacter();

	virtual void Tick(float DeltaTime) override;

	// Called by the PlayerController when the player clicks the ground.
	void SetMoveDestination(const FVector& Destination);

	// Clicked the tree: walk to it, then mine wood while standing in range. 
	void SetTreeTarget(ACaveboundTree* Tree);

protected:
	void StopMining();
	void TryMine(float DeltaTime);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	// Actual camera. Attached to CameraBoom's socket.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	// Placeholder cube so the player is visible before a real character mesh exists.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	// How close (cm) before the character stops walking.
	UPROPERTY(EditAnywhere, Category = "Movement")
	float ArrivalDistance = 80.f;

	FVector MoveDestination = FVector::ZeroVector;
	bool bHasMoveDestination = false;

	// Main tower to mine wood from. Null if we are not mining
	TWeakObjectPtr<ACaveboundTree> TargetTree;

	// Accumulates time while in mine range; resets when we step away
	float MiningTime = 0.f;
};
