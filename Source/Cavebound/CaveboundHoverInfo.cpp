#include "CaveboundHoverInfo.h"
#include "CaveboundUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

void UCaveboundHoverInfo::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// starts hidden until something is hovered
	SetVisibility(ESlateVisibility::Collapsed);

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	// builds the hover card layout
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HoverCanvas"));
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Canvas;

	Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HoverFrame"));
	Frame->SetPadding(FMargin(16.f, 12.f, 16.f, 14.f));
	Frame->SetBrush(CaveboundUIStyle::MakePanelBrush());
	Frame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HoverBody"));
	Frame->SetContent(Body);

	// name label
	NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HoverName"));
	NameText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnWood()));
	NameText->SetFont(CaveboundUIStyle::Font(16));
	NameText->SetShadowColorAndOpacity(FLinearColor(0.12f, 0.08f, 0.06f, 0.5f));
	NameText->SetShadowOffset(FVector2D(1.f, 1.f));
	if (UVerticalBoxSlot* NameSlot = Body->AddChildToVerticalBox(NameText))
	{
		NameSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	// health bar
	USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HoverBarSize"));
	BarSize->SetWidthOverride(168.f);
	BarSize->SetHeightOverride(22.f);
	HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HoverBar"));
	{
		FProgressBarStyle BarStyle = HealthBar->GetWidgetStyle();
		BarStyle.BackgroundImage = CaveboundUIStyle::MakeBarBackgroundBrush();
		BarStyle.FillImage = CaveboundUIStyle::MakeBarFillBrush();
		HealthBar->SetWidgetStyle(BarStyle);
	}
	HealthBar->SetFillColorAndOpacity(FLinearColor::White);
	HealthBar->SetPercent(1.f);
	BarSize->SetContent(HealthBar);
	if (UVerticalBoxSlot* BarSlot = Body->AddChildToVerticalBox(BarSize))
	{
		BarSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	}

	// numeric hp text
	HealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HoverHP"));
	HealthText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::Body()));
	HealthText->SetFont(CaveboundUIStyle::Font(13));
	Body->AddChildToVerticalBox(HealthText);

	USizeBox* CardSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HoverCardSize"));
	CardSizeBox->SetMinDesiredWidth(200.f);
	CardSizeBox->SetContent(Frame);

	PanelSlot = Canvas->AddChildToCanvas(CardSizeBox);
	if (PanelSlot)
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		// top-left of card follows the mouse tip
		PanelSlot->SetAlignment(FVector2D(0.f, 0.f));
		PanelSlot->SetAutoSize(true);
	}
}

void UCaveboundHoverInfo::ShowAt(const FVector2D& ScreenPosition, const FText& Name, float Health, float MaxHealth)
{
	// fills name / hp / bar for the hovered target
	const float MaxHP = FMath::Max(1.f, MaxHealth);
	const float Clamped = FMath::Clamp(Health, 0.f, MaxHP);

	if (NameText)
	{
		NameText->SetText(Name);
	}
	if (HealthText)
	{
		HealthText->SetText(FText::FromString(FString::Printf(
			TEXT("%d / %d"),
			FMath::RoundToInt(Clamped),
			FMath::RoundToInt(MaxHP))));
	}
	if (HealthBar)
	{
		HealthBar->SetPercent(Clamped / MaxHP);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	// measures card so it can flip when near screen edges
	FVector2D CardSize(200.f, 78.f);
	if (Frame)
	{
		Frame->ForceLayoutPrepass();
		const FVector2D Desired = Frame->GetDesiredSize();
		if (Desired.X > 1.f && Desired.Y > 1.f)
		{
			CardSize = Desired;
		}
	}

	FVector2D ViewportSize = GetCachedGeometry().GetLocalSize();
	if (ViewportSize.X < 1.f || ViewportSize.Y < 1.f)
	{
		ViewportSize = FVector2D(1920.f, 1080.f);
		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->GetViewportSize(ViewportSize);
		}
	}

	// keeps the card inside the viewport
	FVector2D Pos = ScreenPosition;
	if (Pos.X + CardSize.X > ViewportSize.X - 4.f)
	{
		Pos.X = ScreenPosition.X - CardSize.X;
	}
	if (Pos.Y + CardSize.Y > ViewportSize.Y - 4.f)
	{
		Pos.Y = ScreenPosition.Y - CardSize.Y;
	}

	Pos.X = FMath::Clamp(Pos.X, 4.f, FMath::Max(4.f, ViewportSize.X - CardSize.X - 4.f));
	Pos.Y = FMath::Clamp(Pos.Y, 4.f, FMath::Max(4.f, ViewportSize.Y - CardSize.Y - 4.f));

	if (PanelSlot)
	{
		PanelSlot->SetPosition(Pos);
	}
}

void UCaveboundHoverInfo::HideInfo()
{
	// hides the hover card
	SetVisibility(ESlateVisibility::Collapsed);
}
