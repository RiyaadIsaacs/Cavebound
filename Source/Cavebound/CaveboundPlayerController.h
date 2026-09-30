#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CaveboundPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;
class UCaveboundBuildMenu;
class UCaveboundBuildPrompt;
class UMaterialInterface;

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

	// B at a free build slot opens the defender picker. The choice calls this.
	bool PlaceDefenderAtSlot(AActor* Slot, TSubclassOf<AActor> DefenderClass);

	void CloseBuildMenu();

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

	void HandleBuildKey();
	void StripSlotPlaceKeys();
	void OpenBuildMenu(AActor* Slot);
	void UpdateBuildSlotPresentation();
	void CollectInteractableSlots(TArray<AActor*>& OutSlots, AActor*& OutClosest) const;
	void SetSlotHighlighted(AActor* Slot, bool bHighlighted);
	bool GetSlotScreenPosition(const AActor* Slot, float Height, FVector2D& OutScreenPosition) const;
	AActor* FindBuildSlotForMenu() const;
	void ClearSlotOccupied(AActor* Slot);

	UFUNCTION()
	void HandlePlacedDefenderDestroyed(AActor* DestroyedActor);

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

	UPROPERTY()
	TObjectPtr<UCaveboundBuildMenu> BuildMenu;

	UPROPERTY()
	TObjectPtr<UCaveboundBuildPrompt> BuildPrompt;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SlotHighlightMaterial;

	TWeakObjectPtr<AActor> MenuSlot;

	TArray<TWeakObjectPtr<AActor>> HighlightedSlots;

	// Defender actor -> the slot it was built on, so a destroyed defender frees the pad.
	UPROPERTY()
	TMap<TObjectPtr<AActor>, TObjectPtr<AActor>> DefendersToSlots;

	bool bOrbitingCamera = false;
	bool bPauseMenuOpen = false;
	float LastOrbitMouseX = 0.f;
	float LastOrbitMouseY = 0.f;
};
