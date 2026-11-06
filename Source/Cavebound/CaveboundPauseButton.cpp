#include "CaveboundPauseButton.h"
#include "CaveboundPlayerController.h"
#include "CaveboundUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

namespace
{
	constexpr float HudEdge = 24.f;
	constexpr int32 HudFont = 18;
}

void UCaveboundPauseButton::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PauseCanvas"));
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Canvas;

	PauseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("PauseButton"));
	CaveboundUIStyle::ApplySquareButtonStyle(PauseButton);

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PauseLabel"));
	Label->SetText(FText::FromString(TEXT("II")));
	Label->SetJustification(ETextJustify::Center);
	Label->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnCream()));
	Label->SetFont(CaveboundUIStyle::Font(HudFont));
	PauseButton->AddChild(Label);
	if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(PauseButton->GetContentSlot()))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Center);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
		ContentSlot->SetPadding(FMargin(0.f));
	}

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PauseSize"));
	Size->SetWidthOverride(64.f);
	Size->SetHeightOverride(64.f);
	Size->SetContent(PauseButton);

	PanelSlot = Canvas->AddChildToCanvas(Size);
	if (PanelSlot)
	{
		PanelSlot->SetAnchors(FAnchors(1.f, 0.f));
		PanelSlot->SetAlignment(FVector2D(1.f, 0.5f));
		PanelSlot->SetPosition(FVector2D(-HudEdge, HudEdge + 32.f));
		PanelSlot->SetAutoSize(true);
	}

	PauseButton->OnClicked.AddDynamic(this, &UCaveboundPauseButton::HandleClicked);
}

void UCaveboundPauseButton::AlignToTreeBarCenter(const FVector2D& TreeBarAbsoluteCenter)
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

	const float LocalY = Geo.AbsoluteToLocal(TreeBarAbsoluteCenter).Y;
	PanelSlot->SetAnchors(FAnchors(1.f, 0.f));
	PanelSlot->SetAlignment(FVector2D(1.f, 0.5f));
	PanelSlot->SetPosition(FVector2D(-HudEdge, LocalY));
}

void UCaveboundPauseButton::HandleClicked()
{
	if (ACaveboundPlayerController* Controller = Cast<ACaveboundPlayerController>(GetOwningPlayer()))
	{
		Controller->TogglePauseMenu();
	}
}
