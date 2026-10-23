#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaveboundResourceReadout.generated.h"

class UTextBlock;
class UCanvasPanelSlot;

/**
 * Small "Rock: N" line under the wood counter.
 */
UCLASS()
class CAVEBOUND_API UCaveboundResourceReadout : public UUserWidget
{
	GENERATED_BODY()

public:
	void Refresh();

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY()
	TObjectPtr<UTextBlock> RockText;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> PanelSlot;
};
