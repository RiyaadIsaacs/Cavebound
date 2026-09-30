#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaveboundBuildPrompt.generated.h"

class UCanvasPanelSlot;

/** Small "B" key hint that sits above a build slot. */
UCLASS()
class CAVEBOUND_API UCaveboundBuildPrompt : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetHoverScreenPosition(FVector2D Position);
	void SetHoverVisible(bool bVisible);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> PanelSlot;
};
