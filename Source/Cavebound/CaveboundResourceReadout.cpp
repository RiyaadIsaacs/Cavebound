#include "CaveboundResourceReadout.h"
#include "CaveboundGameMode.h"
#include "CaveboundUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"

void UCaveboundResourceReadout::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ResourceCanvas"));
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Canvas;

	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResourceFrame"));
	Frame->SetPadding(FMargin(14.f, 10.f));
	Frame->SetBrushColor(CaveboundUIStyle::PanelBg());
	Frame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResourceBody"));
	Frame->SetContent(Body);

	WoodText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WoodText"));
	WoodText->SetText(FText::FromString(TEXT("Wood: 0")));
	WoodText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::Title()));
	WoodText->SetFont(CaveboundUIStyle::Font(16));
	if (UVerticalBoxSlot* WoodSlot = Body->AddChildToVerticalBox(WoodText))
	{
		WoodSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
	}

	RockText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RockText"));
	RockText->SetText(FText::FromString(TEXT("Rock: 0")));
	RockText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::Body()));
	RockText->SetFont(CaveboundUIStyle::Font(16));
	Body->AddChildToVerticalBox(RockText);

	PanelSlot = Canvas->AddChildToCanvas(Frame);
	if (PanelSlot)
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		PanelSlot->SetAlignment(FVector2D(0.f, 0.f));
		PanelSlot->SetPosition(FVector2D(24.f, 24.f));
		PanelSlot->SetAutoSize(true);
	}
}

void UCaveboundResourceReadout::Refresh()
{
	int32 Wood = 0;
	int32 Rock = 0;
	if (UWorld* World = GetWorld())
	{
		if (const ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>())
		{
			Wood = GameMode->GetWood();
			Rock = GameMode->GetRock();
		}
	}

	if (WoodText)
	{
		WoodText->SetText(FText::FromString(FString::Printf(TEXT("Wood: %d"), Wood)));
	}
	if (RockText)
	{
		RockText->SetText(FText::FromString(FString::Printf(TEXT("Rock: %d"), Rock)));
	}
}
