#include "CaveboundResourceReadout.h"
#include "CaveboundGameMode.h"
#include "CaveboundUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
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

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ResourceCanvas"));
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Canvas;

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ResourceRow"));

	auto MakeCounter = [this, Row](const FName TextName, TObjectPtr<UTextBlock>& OutText, const FString& Initial)
	{
		UBorder* Plate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Plate->SetPadding(FMargin(16.f, 8.f, 16.f, 10.f));
		Plate->SetBrush(CaveboundUIStyle::MakeCounterBrush());
		Plate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Plate->SetHorizontalAlignment(HAlign_Center);
		Plate->SetVerticalAlignment(VAlign_Center);

		OutText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextName);
		OutText->SetText(FText::FromString(Initial));
		OutText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnWood()));
		OutText->SetFont(CaveboundUIStyle::Font(18));
		OutText->SetJustification(ETextJustify::Center);
		OutText->SetShadowColorAndOpacity(FLinearColor(0.15f, 0.1f, 0.08f, 0.55f));
		OutText->SetShadowOffset(FVector2D(1.f, 1.f));
		Plate->SetContent(OutText);

		USizeBox* MinWidth = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		MinWidth->SetMinDesiredWidth(132.f);
		MinWidth->SetHeightOverride(44.f);
		MinWidth->SetContent(Plate);

		if (UHorizontalBoxSlot* PlateSlot = Row->AddChildToHorizontalBox(MinWidth))
		{
			PlateSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
			PlateSlot->SetVerticalAlignment(VAlign_Center);
		}
	};

	MakeCounter(TEXT("WoodText"), WoodText, TEXT("Wood  0"));
	MakeCounter(TEXT("RockText"), RockText, TEXT("Rock  0"));

	PanelSlot = Canvas->AddChildToCanvas(Row);
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
		WoodText->SetText(FText::FromString(FString::Printf(TEXT("Wood  %d"), Wood)));
	}
	if (RockText)
	{
		RockText->SetText(FText::FromString(FString::Printf(TEXT("Rock  %d"), Rock)));
	}
}
