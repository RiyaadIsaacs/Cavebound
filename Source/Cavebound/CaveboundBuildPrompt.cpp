#include "CaveboundBuildPrompt.h"
#include "CaveboundUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

void UCaveboundBuildPrompt::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	// builds the floating keycap prompt (B / F)
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PromptCanvas"));
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Canvas;

	UBorder* Keycap = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PromptKeycap"));
	Keycap->SetPadding(FMargin(12.f, 4.f, 12.f, 6.f));
	Keycap->SetBrush(CaveboundUIStyle::MakeCounterBrush());
	Keycap->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Keycap->SetHorizontalAlignment(HAlign_Center);
	Keycap->SetVerticalAlignment(VAlign_Center);

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PromptLabel"));
	PromptLabel = Label;
	Label->SetText(FText::FromString(TEXT("B")));
	Label->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnWood()));
	Label->SetJustification(ETextJustify::Center);
	Label->SetFont(CaveboundUIStyle::Font(20));
	Label->SetShadowColorAndOpacity(FLinearColor(0.15f, 0.1f, 0.08f, 0.55f));
	Label->SetShadowOffset(FVector2D(1.f, 1.f));
	Keycap->SetContent(Label);

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PromptSize"));
	Size->SetMinDesiredWidth(48.f);
	Size->SetHeightOverride(44.f);
	Size->SetContent(Keycap);

	PanelSlot = Canvas->AddChildToCanvas(Size);
	if (PanelSlot)
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 1.f));
		PanelSlot->SetAutoSize(true);
	}
}

void UCaveboundBuildPrompt::SetPromptKey(const FString& Key)
{
	if (PromptLabel)
	{
		PromptLabel->SetText(FText::FromString(Key));
	}
}

void UCaveboundBuildPrompt::SetHoverScreenPosition(FVector2D Position)
{
	// moves the keycap above the nearby slot
	if (PanelSlot)
	{
		PanelSlot->SetPosition(Position);
	}
}

void UCaveboundBuildPrompt::SetHoverVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}
