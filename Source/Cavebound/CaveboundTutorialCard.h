#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaveboundTutorialCard.generated.h"

class UTextBlock;
class UCanvasPanelSlot;

/** Bottom-center tutorial card. Space advances, H skips. */
UCLASS()
class CAVEBOUND_API UCaveboundTutorialCard : public UUserWidget
{
	GENERATED_BODY()

public:
	bool IsActive() const { return bActive; }
	void BeginTutorial();
	void Advance();
	void Skip();

protected:
	virtual void NativeOnInitialized() override;

	void RefreshMessage();
	void Finish();

	UPROPERTY()
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY()
	TObjectPtr<UTextBlock> HintText;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> PanelSlot;

	TArray<FString> Lines;
	int32 LineIndex = 0;
	bool bActive = false;
};
