#include "CaveboundResourceReadout.h"
#include "CaveboundGameMode.h"
#include "CaveboundTree.h"
#include "CaveboundUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"

namespace ResourceReadoutHud
{
	constexpr float Gap = 12.f; // matches speed-button spacing (6 + 6)
	constexpr float Edge = 24.f;
	constexpr int32 FontSize = 18;
}

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

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResourceStack"));

	// tree health card
	UBorder* TreePlate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TreePlate"));
	TreePlate->SetPadding(FMargin(18.f, 12.f, 18.f, 14.f));
	TreePlate->SetBrush(CaveboundUIStyle::MakePanelBrush());
	TreePlate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	UVerticalBox* TreeBody = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TreeBody"));
	TreePlate->SetContent(TreeBody);

	TreeLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TreeLabel"));
	TreeLabel->SetText(FText::FromString(TEXT("Tree")));
	TreeLabel->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnWood()));
	TreeLabel->SetFont(CaveboundUIStyle::Font(ResourceReadoutHud::FontSize));
	TreeLabel->SetShadowColorAndOpacity(FLinearColor(0.15f, 0.1f, 0.08f, 0.55f));
	TreeLabel->SetShadowOffset(FVector2D(1.f, 1.f));
	if (UVerticalBoxSlot* LabelSlot = TreeBody->AddChildToVerticalBox(TreeLabel))
	{
		LabelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TreeBarSize"));
	BarSize->SetWidthOverride(280.f);
	BarSize->SetHeightOverride(28.f);
	TreeHealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("TreeBar"));
	{
		FProgressBarStyle BarStyle = TreeHealthBar->GetWidgetStyle();
		BarStyle.BackgroundImage = CaveboundUIStyle::MakeBarBackgroundBrush();
		BarStyle.FillImage = CaveboundUIStyle::MakeBarFillBrush();
		TreeHealthBar->SetWidgetStyle(BarStyle);
	}
	TreeHealthBar->SetFillColorAndOpacity(FLinearColor::White);
	TreeHealthBar->SetPercent(1.f);
	BarSize->SetContent(TreeHealthBar);
	TreeBody->AddChildToVerticalBox(BarSize);

	TreeHealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TreeHP"));
	TreeHealthText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::Body()));
	TreeHealthText->SetFont(CaveboundUIStyle::Font(ResourceReadoutHud::FontSize));
	TreeHealthText->SetText(FText::FromString(TEXT("100 / 100")));
	if (UVerticalBoxSlot* HpSlot = TreeBody->AddChildToVerticalBox(TreeHealthText))
	{
		HpSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
	}

	if (UVerticalBoxSlot* TreeSlot = Stack->AddChildToVerticalBox(TreePlate))
	{
		TreeSlot->SetPadding(FMargin(0.f, 0.f, 0.f, ResourceReadoutHud::Gap));
		TreeSlot->SetHorizontalAlignment(HAlign_Left);
		TreeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	// wood / rock row — same gap as speed buttons
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ResourceRow"));

	auto MakeCounter = [this, Row](const FName TextName, TObjectPtr<UTextBlock>& OutText, const FString& Initial, bool bLast)
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
		OutText->SetFont(CaveboundUIStyle::Font(ResourceReadoutHud::FontSize));
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
			PlateSlot->SetPadding(bLast ? FMargin(0.f) : FMargin(0.f, 0.f, ResourceReadoutHud::Gap, 0.f));
			PlateSlot->SetVerticalAlignment(VAlign_Center);
		}
	};

	MakeCounter(TEXT("WoodText"), WoodText, TEXT("Wood  0"), false);
	MakeCounter(TEXT("RockText"), RockText, TEXT("Rock  0"), true);
	if (UVerticalBoxSlot* RowSlot = Stack->AddChildToVerticalBox(Row))
	{
		RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, ResourceReadoutHud::Gap));
		RowSlot->SetHorizontalAlignment(HAlign_Left);
		RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	// tip shown when gather / build is blocked during grace — width capped so the tree card doesn't stretch
	GraceTipFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("GraceTipFrame"));
	GraceTipFrame->SetPadding(FMargin(16.f, 10.f, 16.f, 12.f));
	GraceTipFrame->SetBrush(CaveboundUIStyle::MakePanelBrush());
	GraceTipFrame->SetVisibility(ESlateVisibility::Collapsed);

	GraceTipText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GraceTipText"));
	GraceTipText->SetText(FText::FromString(TEXT("You can only gather resources during preparation or combat phase")));
	GraceTipText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnWood()));
	GraceTipText->SetFont(CaveboundUIStyle::Font(ResourceReadoutHud::FontSize));
	GraceTipText->SetAutoWrapText(true);
	GraceTipFrame->SetContent(GraceTipText);

	USizeBox* TipSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("GraceTipSize"));
	TipSize->SetWidthOverride(316.f);
	TipSize->SetContent(GraceTipFrame);
	if (UVerticalBoxSlot* TipSlot = Stack->AddChildToVerticalBox(TipSize))
	{
		TipSlot->SetHorizontalAlignment(HAlign_Left);
		TipSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	PanelSlot = Canvas->AddChildToCanvas(Stack);
	if (PanelSlot)
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		PanelSlot->SetAlignment(FVector2D(0.f, 0.f));
		PanelSlot->SetPosition(FVector2D(ResourceReadoutHud::Edge, ResourceReadoutHud::Edge));
		PanelSlot->SetAutoSize(true);
	}
}

void UCaveboundResourceReadout::ShowGracePhaseTip()
{
	if (GraceTipFrame)
	{
		GraceTipFrame->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UCaveboundResourceReadout::ClearGracePhaseTip()
{
	if (GraceTipFrame)
	{
		GraceTipFrame->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCaveboundResourceReadout::UpdateGracePhaseTip()
{
	// tip only belongs in grace — clear as soon as preparation / combat starts
	const ACaveboundGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ACaveboundGameMode>()
		: nullptr;
	if (!GameMode || !GameMode->IsRoundIdle())
	{
		ClearGracePhaseTip();
	}
}

bool UCaveboundResourceReadout::GetTreeBarCenterAbsolute(FVector2D& OutCenter) const
{
	if (!TreeHealthBar)
	{
		return false;
	}

	const FGeometry Geo = TreeHealthBar->GetCachedGeometry();
	const FVector2D Size = Geo.GetLocalSize();
	if (Size.IsNearlyZero())
	{
		return false;
	}

	OutCenter = Geo.GetAbsolutePosition() + Geo.GetAbsoluteSize() * 0.5f;
	return true;
}

void UCaveboundResourceReadout::Refresh()
{
	int32 Wood = 0;
	int32 Rock = 0;
	float Health = 0.f;
	float MaxHealth = 1.f;

	if (UWorld* World = GetWorld())
	{
		if (const ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>())
		{
			Wood = GameMode->GetWood();
			Rock = GameMode->GetRock();
			if (const ACaveboundTree* Tree = GameMode->GetTree())
			{
				Health = Tree->GetHealth();
				MaxHealth = FMath::Max(1.f, Tree->GetMaxHealth());
			}
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
	if (TreeHealthBar)
	{
		TreeHealthBar->SetPercent(FMath::Clamp(Health / MaxHealth, 0.f, 1.f));
	}
	if (TreeHealthText)
	{
		TreeHealthText->SetText(FText::FromString(FString::Printf(
			TEXT("%d / %d"),
			FMath::RoundToInt(Health),
			FMath::RoundToInt(MaxHealth))));
	}
}
