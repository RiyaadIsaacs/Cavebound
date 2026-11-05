#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaveboundResourceReadout.generated.h"

class UTextBlock;
class UCanvasPanelSlot;

/** Top-left Wood / Rock readout. */
UCLASS()
class CAVEBOUND_API UCaveboundResourceReadout : public UUserWidget
{
	GENERATED_BODY()

public:
	void Refresh();

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY()
	TObjectPtr<UTextBlock> WoodText;

	UPROPERTY()
	TObjectPtr<UTextBlock> RockText;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> PanelSlot;
};
