#include "CaveboundBuildPrompt.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

void UCaveboundBuildPrompt::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PromptCanvas"));
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Canvas;

	UBorder* Keycap = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PromptKeycap"));
	Keycap->SetPadding(FMargin(10.f, 6.f));
	Keycap->SetBrushColor(FLinearColor(0.08f, 0.1f, 0.08f, 0.92f));
	Keycap->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PromptLabel"));
	Label->SetText(FText::FromString(TEXT("B")));
	Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 1.f, 0.75f, 1.f)));
	Label->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = 22;
	Label->SetFont(Font);
	Keycap->SetContent(Label);

	PanelSlot = Canvas->AddChildToCanvas(Keycap);
	if (PanelSlot)
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 1.f));
		PanelSlot->SetAutoSize(true);
	}
}

void UCaveboundBuildPrompt::SetHoverScreenPosition(FVector2D Position)
{
	if (PanelSlot)
	{
		PanelSlot->SetPosition(Position);
	}
}

void UCaveboundBuildPrompt::SetHoverVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}
