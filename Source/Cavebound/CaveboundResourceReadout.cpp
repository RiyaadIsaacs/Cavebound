#include "CaveboundResourceReadout.h"

#include "CaveboundGameMode.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"

void UCaveboundResourceReadout::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RockCanvas"));
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Canvas;

	RockText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RockText"));
	RockText->SetText(FText::FromString(TEXT("Rock: 0")));
	RockText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.74f, 0.66f, 1.f)));
	FSlateFontInfo Font = RockText->GetFont();
	Font.Size = 18;
	RockText->SetFont(Font);

	PanelSlot = Canvas->AddChildToCanvas(RockText);
	if (PanelSlot)
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		PanelSlot->SetAlignment(FVector2D(0.f, 0.f));
		PanelSlot->SetPosition(FVector2D(24.f, 48.f));
		PanelSlot->SetAutoSize(true);
	}
}

void UCaveboundResourceReadout::Refresh()
{
	if (!RockText)
	{
		return;
	}

	int32 Rock = 0;
	if (UWorld* World = GetWorld())
	{
		if (const ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>())
		{
			Rock = GameMode->GetRock();
		}
	}

	RockText->SetText(FText::FromString(FString::Printf(TEXT("Rock: %d"), Rock)));
}
