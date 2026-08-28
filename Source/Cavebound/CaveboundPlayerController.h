#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CaveboundPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;

/**
 * Reads the mouse and tells the Character where to walk.
 */
UCLASS()
class CAVEBOUND_API ACaveboundPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACaveboundPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	// Collection of action mappings (controls and such).
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	// The "click to move" action.
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ClickMoveAction;

	void EnsureClickMoveInput();
	void ShowHUD();

	// For assigning HUD 
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> HUDWidget;

	// Line-trace under the cursor and send that world point to the Character. 
	void OnClickMove();
};
