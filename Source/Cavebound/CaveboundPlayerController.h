#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CaveboundPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;
class UButton;
class UCanvasPanelSlot;
class UCanvasPanel;
class UCaveboundBuildMenu;
class UCaveboundBuildPrompt;
class UCaveboundDefenderMenu;
class UCaveboundHoverInfo;
class UCaveboundDamageVignette;
class UCaveboundResourceReadout;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ACaveboundTurret;
class UHorizontalBox;

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

	// Left-side name for the hover popup (e.g. "Brute")
	UFUNCTION(BlueprintPure, Category = "Cavebound")
	FText GetHoveredDisplayName() const;

	// Right-side "current / max" text for the hover popup
	UFUNCTION(BlueprintPure, Category = "Cavebound")
	FText GetHoveredHealthText() const;

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
	void CloseDefenderMenu();

	// 0 upgrade, 1 repair, 2 disarm. Failure is filled when the action cannot be done.
	bool UseDefenderAction(AActor* Slot, int32 Action, FString& OutFailure);

	// C++ entry for Start Wave (HUD button + Enter key). Avoids broken BP casts after hotreload.
	UFUNCTION(BlueprintCallable, Category = "Cavebound")
	void RequestStartRound();

	/** Soft radial red vignette when the tree is hit. */
	void PulseDamageVignette();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ClickMoveAction;

	void ShowHUD();
	void CreateSkipCountdownButton();
	void UpdateSkipCountdownButton();
	void CreateSpeedButtons();
	void SetGameSpeed(float Speed);
	void RefreshSpeedButtonStyles();
	void UpdateHoverInfoWidget();
	void EnsureDamageVignette();
	AActor* TraceActorUnderCursor() const;
	UFUNCTION()
	void HandleSkipCountdownClicked();
	UFUNCTION()
	void HandleSpeed1Clicked();
	UFUNCTION()
	void HandleSpeed2Clicked();
	UFUNCTION()
	void HandleSpeed3Clicked();
	void ShowPauseMenu();
	void HidePauseMenu();
	void ApplyGameplayInputMode();
	void ApplyMenuInputMode();
	void ApplyPauseInputMode();
	void ConfigureHudClickThrough();
	void ClearEnhancedClickMappings();
	void UpdateHoveredHealthTarget();
	void UpdateHoverGlow();
	void ClearHoverGlow();
	void ApplyHoverGlow(AActor* Actor, const FLinearColor& Color);
	FLinearColor GetHoverGlowColor(const AActor* Actor) const;
	bool IsBuildSlotActor(const AActor* Actor) const;
	bool IsPointerOverInteractiveUI() const;
	bool IsMouseButtonHeld(const FKey& Button) const;
	/** True only if the cursor is over the actor's on-screen silhouette (not a huge collision volume). */
	bool IsCursorOverActorVisual(const AActor* Actor) const;

	void HandleBuildKey();
	void HandleDefenderKey();
	void StripSlotPlaceKeys();
	void OpenBuildMenu(AActor* Slot);
	void OpenDefenderMenu(AActor* Slot);
	void UpdateBuildSlotPresentation();
	void CollectInteractableSlots(TArray<AActor*>& OutSlots, AActor*& OutClosest) const;
	void CollectOccupiedSlots(TArray<AActor*>& OutSlots, AActor*& OutClosest) const;
	ACaveboundTurret* GetDefenderOnSlot(AActor* Slot) const;
	void SetSlotHighlighted(AActor* Slot, bool bHighlighted);
	bool GetSlotScreenPosition(const AActor* Slot, float Height, FVector2D& OutScreenPosition) const;
	AActor* FindBuildSlotForMenu() const;
	void ClearSlotOccupied(AActor* Slot);
	// Spawns and registers the defender immediately so the beam can fade it in.
	AActor* SpawnPlacedDefender(AActor* Slot, TSubclassOf<AActor> DefenderClass, const FTransform& SlotTransform);

	UFUNCTION()
	void HandlePlacedDefenderDestroyed(AActor* DestroyedActor);

	// For assigning HUD 
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> HUDWidget;

	UPROPERTY()
	TObjectPtr<UCanvasPanel> SkipCountdownCanvas;

	UPROPERTY()
	TObjectPtr<UButton> SkipCountdownButton;

	UPROPERTY()
	TObjectPtr<UButton> StartWaveButton;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> SkipCountdownButtonSlot;

	UPROPERTY()
	TObjectPtr<UButton> SpeedButton1;

	UPROPERTY()
	TObjectPtr<UButton> SpeedButton2;

	UPROPERTY()
	TObjectPtr<UButton> SpeedButton3;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> SpeedButtonsSlot;

	float GameSpeed = 1.f;

	// Separate from WBP_PlayerHUD so pause can block input without breaking click-to-move
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> PauseMenuWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> PauseMenuWidget;

	// Line-trace under the cursor and send that world point to the Character
	void OnClickMove();

	TWeakObjectPtr<AActor> HoveredHealthActor;
	TWeakObjectPtr<AActor> HoveredBuildSlot;
	TWeakObjectPtr<AActor> HoverGlowActor;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> HoverGlowMaterial;

	/** Flat tint overlay — used for skeletal enemies / Nanite-prone rocks where rim outline fails. */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> HoverGlowFillMaterial;

	UPROPERTY()
	TObjectPtr<UCaveboundBuildMenu> BuildMenu;

	UPROPERTY()
	TObjectPtr<UCaveboundDefenderMenu> DefenderMenu;

	UPROPERTY()
	TObjectPtr<UCaveboundBuildPrompt> BuildPrompt;

	UPROPERTY()
	TObjectPtr<UCaveboundResourceReadout> RockReadout;

	UPROPERTY()
	TObjectPtr<UCaveboundHoverInfo> HoverInfo;

	UPROPERTY()
	TObjectPtr<UCaveboundDamageVignette> DamageVignette;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SlotHighlightMaterial;

	TWeakObjectPtr<AActor> MenuSlot;

	TArray<TWeakObjectPtr<AActor>> HighlightedSlots;

	// Defender actor -> the slot it was built on, so a destroyed defender frees the pad.
	UPROPERTY()
	TMap<TObjectPtr<AActor>, TObjectPtr<AActor>> DefendersToSlots;

	bool bOrbitingCamera = false;
	bool bPauseMenuOpen = false;
	bool bLeftMouseWasDown = false;
	bool bRightMouseWasDown = false;
	float LastOrbitMouseX = 0.f;
	float LastOrbitMouseY = 0.f;
};
