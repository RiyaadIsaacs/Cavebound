#include "CaveboundTutorialCard.h"
#include "CaveboundUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace TutorialCardHud
{
	constexpr float Edge = 24.f;
	constexpr float CardWidth = 1100.f;
	constexpr int32 MessageFont = 18;
	constexpr int32 HintFont = 13;
}

void UCaveboundTutorialCard::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Lines = {
		TEXT("Welcome to Cavebound"),
		TEXT("Your goal is simple, protect the tree of life whilst fending off enemy hordes"),
		TEXT("Gather wood from the tree by clicking on it and gather rocks by clicking on the deposits scattered around"),
		TEXT("These resources can be used to build defenders at the build slots"),
		TEXT("Built defenders may also be upgraded and repaired at an expense"),
		TEXT("You can only collect resources during the preperation or combat phase"),
		TEXT("You can advance to the preperation phase by pressing the new wave button on the right"),
		TEXT("You can also increase the gameplay speed if you feel things are moving too slow by pressing the buttons below the new wave"),
		TEXT("Good luck and have fun"),
	};

	SetVisibility(ESlateVisibility::Collapsed);

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TutorialCanvas"));
	Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = Canvas;

	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TutorialFrame"));
	Frame->SetPadding(FMargin(28.f, 20.f, 28.f, 22.f));
	Frame->SetBrush(CaveboundUIStyle::MakePanelBrush());
	Frame->SetVisibility(ESlateVisibility::HitTestInvisible);

	UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TutorialBody"));
	Frame->SetContent(Body);

	MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TutorialMessage"));
	MessageText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnWood()));
	MessageText->SetFont(CaveboundUIStyle::Font(TutorialCardHud::MessageFont));
	MessageText->SetJustification(ETextJustify::Center);
	MessageText->SetAutoWrapText(true);
	MessageText->SetWrappingPolicy(ETextWrappingPolicy::DefaultWrapping);
	MessageText->SetShadowColorAndOpacity(FLinearColor(0.12f, 0.08f, 0.06f, 0.5f));
	MessageText->SetShadowOffset(FVector2D(1.f, 1.f));
	if (UVerticalBoxSlot* MessageSlot = Body->AddChildToVerticalBox(MessageText))
	{
		MessageSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));
		MessageSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	UHorizontalBox* HintRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TutorialHints"));

	UTextBlock* SkipHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TutorialSkipHint"));
	SkipHint->SetText(FText::FromString(TEXT("H to skip tutorial")));
	SkipHint->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::Body()));
	SkipHint->SetFont(CaveboundUIStyle::Font(TutorialCardHud::HintFont));
	SkipHint->SetJustification(ETextJustify::Left);
	if (UHorizontalBoxSlot* SkipSlot = HintRow->AddChildToHorizontalBox(SkipHint))
	{
		SkipSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SkipSlot->SetHorizontalAlignment(HAlign_Left);
		SkipSlot->SetVerticalAlignment(VAlign_Center);
	}
	HintText = SkipHint;

	UTextBlock* AdvanceHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TutorialAdvanceHint"));
	AdvanceHint->SetText(FText::FromString(TEXT("Space to advance")));
	AdvanceHint->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::Body()));
	AdvanceHint->SetFont(CaveboundUIStyle::Font(TutorialCardHud::HintFont));
	AdvanceHint->SetJustification(ETextJustify::Right);
	if (UHorizontalBoxSlot* AdvanceSlot = HintRow->AddChildToHorizontalBox(AdvanceHint))
	{
		AdvanceSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		AdvanceSlot->SetHorizontalAlignment(HAlign_Right);
		AdvanceSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (UVerticalBoxSlot* HintSlot = Body->AddChildToVerticalBox(HintRow))
	{
		HintSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TutorialSize"));
	Size->SetWidthOverride(TutorialCardHud::CardWidth);
	Size->SetContent(Frame);

	PanelSlot = Canvas->AddChildToCanvas(Size);
	if (PanelSlot)
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 1.f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 1.f));
		PanelSlot->SetPosition(FVector2D(0.f, -TutorialCardHud::Edge));
		PanelSlot->SetAutoSize(true);
	}
}

void UCaveboundTutorialCard::BeginTutorial()
{
	LineIndex = 0;
	bActive = true;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	RefreshMessage();
}

void UCaveboundTutorialCard::Advance()
{
	if (!bActive)
	{
		return;
	}

	++LineIndex;
	if (LineIndex >= Lines.Num())
	{
		Finish();
		return;
	}

	RefreshMessage();
}

void UCaveboundTutorialCard::Skip()
{
	if (!bActive)
	{
		return;
	}

	Finish();
}

void UCaveboundTutorialCard::RefreshMessage()
{
	if (!MessageText)
	{
		return;
	}

	if (Lines.IsValidIndex(LineIndex))
	{
		MessageText->SetText(FText::FromString(Lines[LineIndex]));
	}
}

void UCaveboundTutorialCard::Finish()
{
	bActive = false;
	LineIndex = Lines.Num();
	SetVisibility(ESlateVisibility::Collapsed);
}
