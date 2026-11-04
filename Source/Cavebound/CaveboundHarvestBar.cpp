#include "CaveboundHarvestBar.h"
#include "CaveboundUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UCaveboundHarvestBar::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HarvestColumn"));
	WidgetTree->RootWidget = Column;

	AmountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HarvestAmount"));
	AmountText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnWood()));
	AmountText->SetFont(CaveboundUIStyle::Font(14));
	AmountText->SetJustification(ETextJustify::Center);
	AmountText->SetShadowColorAndOpacity(FLinearColor(0.12f, 0.08f, 0.06f, 0.55f));
	AmountText->SetShadowOffset(FVector2D(1.f, 1.f));
	if (UVerticalBoxSlot* AmountSlot = Column->AddChildToVerticalBox(AmountText))
	{
		AmountSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		AmountSlot->SetHorizontalAlignment(HAlign_Center);
	}

	USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HarvestBarSize"));
	BarSize->SetWidthOverride(140.f);
	BarSize->SetHeightOverride(20.f);

	Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HarvestBar"));
	{
		FProgressBarStyle BarStyle = Bar->GetWidgetStyle();
		BarStyle.BackgroundImage = CaveboundUIStyle::MakeBarBackgroundBrush();
		BarStyle.FillImage = CaveboundUIStyle::MakeBarFillBrush();
		Bar->SetWidgetStyle(BarStyle);
	}
	Bar->SetFillColorAndOpacity(FLinearColor::White);
	Bar->SetPercent(0.f);
	BarSize->SetContent(Bar);
	Column->AddChildToVerticalBox(BarSize);
}

void UCaveboundHarvestBar::SetHarvest(int32 RockAmount, float Progress)
{
	if (AmountText)
	{
		AmountText->SetText(FText::FromString(FString::Printf(TEXT("%d rock"), RockAmount)));
	}

	if (Bar)
	{
		Bar->SetPercent(FMath::Clamp(Progress, 0.f, 1.f));
	}
}
