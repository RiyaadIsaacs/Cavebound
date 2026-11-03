#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaveboundHoverInfo.generated.h"

class UBorder;
class UCanvasPanelSlot;
class UProgressBar;
class UTextBlock;

/** Name + HP bar that follows the cursor when hovering a health target. */
UCLASS()
class CAVEBOUND_API UCaveboundHoverInfo : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowAt(const FVector2D& ScreenPosition, const FText& Name, float Health, float MaxHealth);
	void HideInfo();

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> PanelSlot;

	UPROPERTY()
	TObjectPtr<UBorder> Frame;

	UPROPERTY()
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY()
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY()
	TObjectPtr<UProgressBar> HealthBar;
};
