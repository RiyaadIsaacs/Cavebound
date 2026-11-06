#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaveboundGameplayHud.generated.h"

class UCanvasPanel;

/** C++ HUD root (Start Wave / speed / skip live on this canvas). Replaces WBP_PlayerHUD. */
UCLASS()
class CAVEBOUND_API UCaveboundGameplayHud : public UUserWidget
{
	GENERATED_BODY()

public:
	UCanvasPanel* GetRootCanvas() const { return RootCanvas; }

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY()
	TObjectPtr<UCanvasPanel> RootCanvas;
};
