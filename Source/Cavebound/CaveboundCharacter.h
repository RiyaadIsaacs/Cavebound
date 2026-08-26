#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CaveboundCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;

/**
  The player's body in the world: capsule collision, a visible mesh, walking and an almost top-down camera that follows automatically.
 */
UCLASS()
class CAVEBOUND_API ACaveboundCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACaveboundCharacter();

	virtual void Tick(float DeltaTime) override;

	// Called by the PlayerController when the player clicks the world.
	void SetMoveDestination(const FVector& Destination);

protected:
	// Camera stick from the capsule.
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

	// World location we are walking toward. 
	FVector MoveDestination = FVector::ZeroVector;

	// False until the first click.
	bool bHasMoveDestination = false;
};
