#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaveboundPauseButton.generated.h"

class UButton;
class UCanvasPanelSlot;

/** Top-right pause button, vertically aligned with the tree health bar. */
UCLASS()
class CAVEBOUND_API UCaveboundPauseButton : public UUserWidget
{
	GENERATED_BODY()

public:
	void AlignToTreeBarCenter(const FVector2D& TreeBarAbsoluteCenter);
	UButton* GetButton() const { return PauseButton; }

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION()
	void HandleClicked();

	UPROPERTY()
	TObjectPtr<UButton> PauseButton;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> PanelSlot;
};
