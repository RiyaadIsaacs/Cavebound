#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaveboundResourceReadout.generated.h"

class UBorder;
class UProgressBar;
class UTextBlock;
class UCanvasPanelSlot;

/** Top-left tree health bar with Wood / Rock counters underneath. */
UCLASS()
class CAVEBOUND_API UCaveboundResourceReadout : public UUserWidget
{
	GENERATED_BODY()

public:
	void Refresh();
	void ShowGraceBuildTip();
	void TickGraceTip(float DeltaTime);

	/** Absolute Y of the tree-health row center — used to align the pause button. */
	bool GetTreeBarCenterAbsolute(FVector2D& OutCenter) const;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY()
	TObjectPtr<UTextBlock> TreeLabel;

	UPROPERTY()
	TObjectPtr<UProgressBar> TreeHealthBar;

	UPROPERTY()
	TObjectPtr<UTextBlock> TreeHealthText;

	UPROPERTY()
	TObjectPtr<UTextBlock> WoodText;

	UPROPERTY()
	TObjectPtr<UTextBlock> RockText;

	UPROPERTY()
	TObjectPtr<UBorder> GraceTipFrame;

	UPROPERTY()
	TObjectPtr<UTextBlock> GraceTipText;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> PanelSlot;

	float GraceTipLife = 0.f;
};
