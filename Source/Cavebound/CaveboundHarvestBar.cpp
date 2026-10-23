#include "CaveboundHarvestBar.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
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
	AmountText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.82f, 0.75f, 1.f)));
	AmountText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* AmountSlot = Column->AddChildToVerticalBox(AmountText))
	{
		AmountSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	}

	Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HarvestBar"));
	Bar->SetFillColorAndOpacity(FLinearColor(0.62f, 0.55f, 0.42f, 1.f));
	Bar->SetPercent(0.f);
	Column->AddChildToVerticalBox(Bar);
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
