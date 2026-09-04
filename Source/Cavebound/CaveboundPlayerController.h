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

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	bool HasHoveredHealthTarget() const;

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	float GetHoveredHealth() const;

	UFUNCTION(BlueprintPure, Category = "Cavebound")
	float GetHoveredMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Cavebound|Pause")
	bool IsPauseMenuOpen() const { return bPauseMenuOpen; }

	// Escape / Tab 
	UFUNCTION(BlueprintCallable, Category = "Cavebound|Pause")
	void TogglePauseMenu();

	UFUNCTION(BlueprintCallable, Category = "Cavebound|Pause")
	void PauseGame();

	UFUNCTION(BlueprintCallable, Category = "Cavebound|Pause")
	void ResumeGame();

	// Quit button on the pause menu
	UFUNCTION(BlueprintCallable, Category = "Cavebound|Pause")
	void QuitGame();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	// Collection of action mappings
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	// The "click to move" action
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ClickMoveAction;

	void EnsureClickMoveInput();
	void ShowHUD();
	void ShowPauseMenu();
	void HidePauseMenu();
	void ApplyGameplayInputMode();
	void ApplyPauseInputMode();
	void UpdateHoveredHealthTarget();

	// For assigning HUD 
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> HUDWidget;

	// Separate from WBP_PlayerHUD so pause can block input without breaking click-to-move
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> PauseMenuWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> PauseMenuWidget;

	// Line-trace under the cursor and send that world point to the Character
	void OnClickMove();

	TWeakObjectPtr<AActor> HoveredHealthActor;

	bool bOrbitingCamera = false;
	bool bPauseMenuOpen = false;
	float LastOrbitMouseX = 0.f;
	float LastOrbitMouseY = 0.f;
};
