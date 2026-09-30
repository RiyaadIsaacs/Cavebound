#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaveboundBuildMenu.generated.h"

class UButton;
class UCanvasPanelSlot;
class UTextBlock;
class UVerticalBox;

/** Remembers which defender a menu button builds. */
UCLASS()
class UCaveboundBuildChoiceProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TSubclassOf<AActor> DefenderClass;

	UPROPERTY()
	TObjectPtr<class UCaveboundBuildMenu> Menu;

	UFUNCTION()
	void HandleClicked();
};

/**
 * Small picker shown at a build slot. Lists defender types and their wood cost.
 */
UCLASS()
class CAVEBOUND_API UCaveboundBuildMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	void OpenForSlot(AActor* Slot);
	void SetHoverScreenPosition(FVector2D Position);
	void SetHoverVisible(bool bVisible);

protected:
	virtual void NativeOnInitialized() override;

	void AddChoice(const FString& Label, TSubclassOf<AActor> DefenderClass);

	UPROPERTY()
	TObjectPtr<UVerticalBox> ChoiceBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> WoodText;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> PanelSlot;

	UPROPERTY()
	TArray<TObjectPtr<UCaveboundBuildChoiceProxy>> ChoiceProxies;

	TWeakObjectPtr<AActor> TargetSlot;

	friend class UCaveboundBuildChoiceProxy;
	void SelectDefender(TSubclassOf<AActor> DefenderClass);
};
