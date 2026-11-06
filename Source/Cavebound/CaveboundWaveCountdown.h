#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaveboundWaveCountdown.generated.h"

class UTextBlock;
class UCanvasPanelSlot;

/** Top-center next-wave / collection countdown card. */
UCLASS()
class CAVEBOUND_API UCaveboundWaveCountdown : public UUserWidget
{
	GENERATED_BODY()

public:
	void Refresh();

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY()
	TObjectPtr<UTextBlock> TimeText;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> PanelSlot;
};
