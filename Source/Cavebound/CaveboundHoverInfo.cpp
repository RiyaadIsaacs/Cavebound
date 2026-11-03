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

void UCaveboundHoverInfo::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::Collapsed);

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HoverCanvas"));
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Canvas;

	Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HoverFrame"));
	Frame->SetPadding(FMargin(12.f, 8.f));
	Frame->SetBrushColor(CaveboundUIStyle::PanelBg());
	Frame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HoverBody"));
	Frame->SetContent(Body);

	NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HoverName"));
	NameText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::Title()));
	NameText->SetFont(CaveboundUIStyle::Font(16));
	if (UVerticalBoxSlot* NameSlot = Body->AddChildToVerticalBox(NameText))
	{
		NameSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	}

	USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HoverBarSize"));
	BarSize->SetWidthOverride(160.f);
	BarSize->SetHeightOverride(12.f);
	HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HoverBar"));
	HealthBar->SetFillColorAndOpacity(CaveboundUIStyle::HealthFill());
	HealthBar->SetPercent(1.f);
	BarSize->SetContent(HealthBar);
	if (UVerticalBoxSlot* BarSlot = Body->AddChildToVerticalBox(BarSize))
	{
		BarSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	}

	HealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HoverHP"));
	HealthText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::Body()));
	HealthText->SetFont(CaveboundUIStyle::Font(14));
	Body->AddChildToVerticalBox(HealthText);

	PanelSlot = Canvas->AddChildToCanvas(Frame);
	if (PanelSlot)
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		PanelSlot->SetAlignment(FVector2D(0.f, 1.f));
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetSize(FVector2D(180.f, 70.f));
	}
}

void UCaveboundHoverInfo::ShowAt(const FVector2D& ScreenPosition, const FText& Name, float Health, float MaxHealth)
{
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
	if (PanelSlot)
	{
		// Sit just above the cursor.
		PanelSlot->SetPosition(ScreenPosition + FVector2D(18.f, -12.f));
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UCaveboundHoverInfo::HideInfo()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
