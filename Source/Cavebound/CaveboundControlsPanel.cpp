#include "CaveboundControlsPanel.h"
#include "CaveboundUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	constexpr int32 ControlsFontSize = 18;
	constexpr float ExpandedMinWidth = 560.f;
}

void UCaveboundControlsPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ControlsCanvas"));
	Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = Canvas;

	Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ControlsFrame"));
	Frame->SetPadding(FMargin(36.f, 28.f, 36.f, 32.f));
	Frame->SetBrush(CaveboundUIStyle::MakePanelBrush());
	Frame->SetVisibility(ESlateVisibility::HitTestInvisible);

	UVerticalBox* RootBody = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ControlsRoot"));
	Frame->SetContent(RootBody);

	// expanded list
	UVerticalBox* Expanded = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ControlsExpanded"));
	ExpandedBody = Expanded;

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ControlsTitle"));
	Title->SetText(FText::FromString(TEXT("Controls")));
	Title->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnWood()));
	Title->SetFont(CaveboundUIStyle::Font(ControlsFontSize));
	Title->SetShadowColorAndOpacity(FLinearColor(0.12f, 0.08f, 0.06f, 0.5f));
	Title->SetShadowOffset(FVector2D(1.f, 1.f));
	if (UVerticalBoxSlot* TitleSlot = Expanded->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ControlsHint"));
	Hint->SetText(FText::FromString(TEXT("Press Tab to hide")));
	Hint->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::Body()));
	Hint->SetFont(CaveboundUIStyle::Font(ControlsFontSize));
	if (UVerticalBoxSlot* HintSlot = Expanded->AddChildToVerticalBox(Hint))
	{
		HintSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
	}

	AddRow(Expanded, TEXT("Left click"), TEXT("Move / harvest resources"));
	AddRow(Expanded, TEXT("Hold right click"), TEXT("Rotate camera"));
	AddRow(Expanded, TEXT("B when near build slot"), TEXT("Build defender"));
	AddRow(Expanded, TEXT("F when near defender"), TEXT("Upgrade / repair"));
	AddRow(Expanded, TEXT("Enter"), TEXT("Start next wave"));
	AddRow(Expanded, TEXT("Esc"), TEXT("Pause"));
	RootBody->AddChildToVerticalBox(Expanded);

	// collapsed one-liner
	CollapsedLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ControlsCollapsed"));
	CollapsedLabel->SetText(FText::FromString(TEXT("Controls  (press Tab to open)")));
	CollapsedLabel->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnWood()));
	CollapsedLabel->SetFont(CaveboundUIStyle::Font(ControlsFontSize));
	CollapsedLabel->SetJustification(ETextJustify::Center);
	CollapsedLabel->SetShadowColorAndOpacity(FLinearColor(0.12f, 0.08f, 0.06f, 0.5f));
	CollapsedLabel->SetShadowOffset(FVector2D(1.f, 1.f));
	RootBody->AddChildToVerticalBox(CollapsedLabel);

	PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ControlsSize"));
	PanelSize->SetMinDesiredWidth(ExpandedMinWidth);
	PanelSize->SetContent(Frame);

	PanelSlot = Canvas->AddChildToCanvas(PanelSize);
	if (PanelSlot)
	{
		PanelSlot->SetAnchors(FAnchors(1.f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(1.f, 0.5f));
		PanelSlot->SetPosition(FVector2D(-24.f, 0.f));
		PanelSlot->SetAutoSize(true);
	}

	RefreshCollapsedState();
}

void UCaveboundControlsPanel::SetMinWidth(float MinWidth)
{
	if (PanelSize)
	{
		PanelSize->SetMinDesiredWidth(bCollapsed ? FMath::Max(280.f, MinWidth * 0.55f) : FMath::Max(ExpandedMinWidth, MinWidth));
	}
}

void UCaveboundControlsPanel::AlignRightEdgeToAbsolute(float AbsoluteRightX)
{
	if (!PanelSlot)
	{
		return;
	}

	const FGeometry Geo = GetCachedGeometry();
	const FVector2D LocalSize = Geo.GetLocalSize();
	if (LocalSize.X < 1.f || LocalSize.Y < 1.f)
	{
		return;
	}

	const float LocalRight = Geo.AbsoluteToLocal(FVector2D(AbsoluteRightX, 0.f)).X;
	PanelSlot->SetAnchors(FAnchors(1.f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(1.f, 0.5f));
	PanelSlot->SetPosition(FVector2D(LocalRight - LocalSize.X, 0.f));
}

void UCaveboundControlsPanel::ToggleCollapsed()
{
	SetCollapsed(!bCollapsed);
}

void UCaveboundControlsPanel::SetCollapsed(bool bInCollapsed)
{
	bCollapsed = bInCollapsed;
	RefreshCollapsedState();
}

void UCaveboundControlsPanel::RefreshCollapsedState()
{
	if (ExpandedBody)
	{
		ExpandedBody->SetVisibility(bCollapsed ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (CollapsedLabel)
	{
		CollapsedLabel->SetVisibility(bCollapsed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (Frame)
	{
		Frame->SetPadding(bCollapsed ? FMargin(22.f, 14.f, 22.f, 16.f) : FMargin(36.f, 28.f, 36.f, 32.f));
	}
	if (PanelSize)
	{
		PanelSize->SetMinDesiredWidth(bCollapsed ? 280.f : ExpandedMinWidth);
	}
}

void UCaveboundControlsPanel::AddRow(UVerticalBox* Body, const FString& Key, const FString& Action)
{
	if (!Body || !WidgetTree)
	{
		return;
	}

	// key on its own line, action below — clearer spacing for long labels
	UVerticalBox* Entry = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	UTextBlock* KeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	KeyText->SetText(FText::FromString(Key));
	KeyText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnWood()));
	KeyText->SetFont(CaveboundUIStyle::Font(ControlsFontSize));
	KeyText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* KeySlot = Entry->AddChildToVerticalBox(KeyText))
	{
		KeySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	}

	UTextBlock* ActionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	ActionText->SetText(FText::FromString(Action));
	ActionText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::Body()));
	ActionText->SetFont(CaveboundUIStyle::Font(ControlsFontSize));
	ActionText->SetAutoWrapText(true);
	Entry->AddChildToVerticalBox(ActionText);

	if (UVerticalBoxSlot* RowSlot = Body->AddChildToVerticalBox(Entry))
	{
		RowSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 8.f));
	}
}
