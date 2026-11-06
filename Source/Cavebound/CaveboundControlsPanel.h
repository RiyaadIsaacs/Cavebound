#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaveboundControlsPanel.generated.h"

class UBorder;
class UCanvasPanelSlot;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWidget;

/** Right-side panel listing the game's controls. Tab toggles expand / collapse. */
UCLASS()
class CAVEBOUND_API UCaveboundControlsPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	void AlignRightEdgeToAbsolute(float AbsoluteRightX);
	void SetMinWidth(float MinWidth);
	void ToggleCollapsed();
	void SetCollapsed(bool bInCollapsed);
	bool IsCollapsed() const { return bCollapsed; }

protected:
	virtual void NativeOnInitialized() override;

	void AddRow(UVerticalBox* Body, const FString& Key, const FString& Action);
	void RefreshCollapsedState();

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> PanelSlot;

	UPROPERTY()
	TObjectPtr<USizeBox> PanelSize;

	UPROPERTY()
	TObjectPtr<UBorder> Frame;

	UPROPERTY()
	TObjectPtr<UWidget> ExpandedBody;

	UPROPERTY()
	TObjectPtr<UTextBlock> CollapsedLabel;

	bool bCollapsed = false;
};
