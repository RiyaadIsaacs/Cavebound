#include "CaveboundWaveCountdown.h"
#include "CaveboundGameMode.h"
#include "CaveboundUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"

namespace
{
	constexpr float HudEdge = 24.f;
	constexpr int32 HudFont = 18;
}

void UCaveboundWaveCountdown::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WaveCanvas"));
	Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = Canvas;

	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WaveFrame"));
	Frame->SetPadding(FMargin(22.f, 12.f, 22.f, 14.f));
	Frame->SetBrush(CaveboundUIStyle::MakePanelBrush());
	Frame->SetVisibility(ESlateVisibility::HitTestInvisible);

	UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("WaveBody"));
	Frame->SetContent(Body);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WaveTitle"));
	TitleText->SetText(FText::FromString(TEXT("Grace Period")));
	TitleText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnWood()));
	TitleText->SetFont(CaveboundUIStyle::Font(HudFont));
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetAutoWrapText(true);
	TitleText->SetShadowColorAndOpacity(FLinearColor(0.12f, 0.08f, 0.06f, 0.5f));
	TitleText->SetShadowOffset(FVector2D(1.f, 1.f));
	if (UVerticalBoxSlot* TitleSlot = Body->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
	}

	TimeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WaveTime"));
	TimeText->SetText(FText::FromString(TEXT("Start new wave to enter preparation phase")));
	TimeText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::Body()));
	TimeText->SetFont(CaveboundUIStyle::Font(HudFont));
	TimeText->SetJustification(ETextJustify::Center);
	TimeText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* TimeSlot = Body->AddChildToVerticalBox(TimeText))
	{
		TimeSlot->SetHorizontalAlignment(HAlign_Center);
	}

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("WaveSize"));
	Size->SetMinDesiredWidth(420.f);
	Size->SetContent(Frame);

	PanelSlot = Canvas->AddChildToCanvas(Size);
	if (PanelSlot)
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.f));
		PanelSlot->SetPosition(FVector2D(0.f, HudEdge));
		PanelSlot->SetAutoSize(true);
	}
}

void UCaveboundWaveCountdown::Refresh()
{
	if (!TitleText || !TimeText)
	{
		return;
	}

	const ACaveboundGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ACaveboundGameMode>()
		: nullptr;
	if (!GameMode)
	{
		TitleText->SetText(FText::FromString(TEXT("Grace Period")));
		TimeText->SetText(FText::FromString(TEXT("Start new wave to enter preparation phase")));
		return;
	}

	switch (GameMode->GetRoundState())
	{
	case ECaveboundRoundState::Collecting:
	{
		TitleText->SetText(FText::FromString(TEXT("Preparation Phase")));
		const float Remaining = GameMode->GetCollectionTimeRemaining();
		const int32 Seconds = FMath::Max(0, FMath::CeilToInt(Remaining));
		TimeText->SetText(FText::FromString(FString::Printf(TEXT("New wave begins in %d"), Seconds)));
		break;
	}
	case ECaveboundRoundState::Combat:
		TitleText->SetText(FText::FromString(TEXT("Combat")));
		TimeText->SetText(FText::FromString(TEXT("Wave in progress")));
		break;
	case ECaveboundRoundState::GameOver:
		TitleText->SetText(FText::FromString(TEXT("Game Over")));
		TimeText->SetText(FText::FromString(TEXT("The tree has fallen")));
		break;
	case ECaveboundRoundState::Idle:
	default:
		TitleText->SetText(FText::FromString(TEXT("Grace Period")));
		TimeText->SetText(FText::FromString(TEXT("Start new wave to enter preparation phase")));
		break;
	}
}
