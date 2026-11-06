#include "CaveboundPauseMenu.h"
#include "CaveboundPlayerController.h"
#include "CaveboundUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UCaveboundPauseActionProxy::HandleClicked()
{
	if (!Menu)
	{
		return;
	}

	switch (Action)
	{
	case 0: Menu->HandleResume(); break;
	case 1: Menu->HandleRestart(); break;
	case 2: Menu->HandleQuit(); break;
	default: break;
	}
}

void UCaveboundPauseMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::Visible);

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PauseCanvas"));
	Canvas->SetVisibility(ESlateVisibility::Visible);
	WidgetTree->RootWidget = Canvas;

	UBorder* Dim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PauseDim"));
	Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.55f));
	Dim->SetVisibility(ESlateVisibility::Visible);
	if (UCanvasPanelSlot* DimSlot = Canvas->AddChildToCanvas(Dim))
	{
		DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		DimSlot->SetOffsets(FMargin(0.f));
	}

	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PauseFrame"));
	Frame->SetPadding(FMargin(28.f, 24.f, 28.f, 28.f));
	Frame->SetBrush(CaveboundUIStyle::MakePanelBrush());

	UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PauseBody"));
	Frame->SetContent(Body);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PauseTitle"));
	Title->SetText(FText::FromString(TEXT("Paused")));
	Title->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnWood()));
	Title->SetFont(CaveboundUIStyle::Font(18));
	Title->SetJustification(ETextJustify::Center);
	Title->SetShadowColorAndOpacity(FLinearColor(0.12f, 0.08f, 0.06f, 0.5f));
	Title->SetShadowOffset(FVector2D(1.f, 1.f));
	if (UVerticalBoxSlot* TitleSlot = Body->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
	}

	AddActionButton(Body, TEXT("Resume"), 0);
	AddActionButton(Body, TEXT("Restart"), 1);
	AddActionButton(Body, TEXT("Quit"), 2);

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PauseSize"));
	Size->SetMinDesiredWidth(280.f);
	Size->SetContent(Frame);

	if (UCanvasPanelSlot* FrameSlot = Canvas->AddChildToCanvas(Size))
	{
		FrameSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		FrameSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		FrameSlot->SetAutoSize(true);
	}
}

void UCaveboundPauseMenu::AddActionButton(UVerticalBox* Body, const FString& Label, int32 Action)
{
	if (!Body || !WidgetTree)
	{
		return;
	}

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CaveboundUIStyle::ApplyRectButtonStyle(Button);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Text->SetText(FText::FromString(Label));
	Text->SetJustification(ETextJustify::Center);
	Text->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnCream()));
	Text->SetFont(CaveboundUIStyle::Font(18));
	Button->AddChild(Text);
	if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Button->GetContentSlot()))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Center);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
		ContentSlot->SetPadding(FMargin(4.f, 0.f));
	}

	UCaveboundPauseActionProxy* Proxy = NewObject<UCaveboundPauseActionProxy>(this);
	Proxy->Menu = this;
	Proxy->Action = Action;
	ButtonProxies.Add(Proxy);
	Button->OnClicked.AddDynamic(Proxy, &UCaveboundPauseActionProxy::HandleClicked);

	USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ButtonSize->SetHeightOverride(72.f);
	ButtonSize->SetWidthOverride(240.f);
	ButtonSize->SetContent(Button);

	if (UVerticalBoxSlot* ButtonSlot = Body->AddChildToVerticalBox(ButtonSize))
	{
		ButtonSlot->SetPadding(FMargin(0.f, 6.f));
		ButtonSlot->SetHorizontalAlignment(HAlign_Center);
	}
}

void UCaveboundPauseMenu::HandleResume()
{
	if (ACaveboundPlayerController* Controller = Cast<ACaveboundPlayerController>(GetOwningPlayer()))
	{
		Controller->ResumeGame();
	}
}

void UCaveboundPauseMenu::HandleRestart()
{
	if (ACaveboundPlayerController* Controller = Cast<ACaveboundPlayerController>(GetOwningPlayer()))
	{
		Controller->RestartMatch();
	}
}

void UCaveboundPauseMenu::HandleQuit()
{
	if (ACaveboundPlayerController* Controller = Cast<ACaveboundPlayerController>(GetOwningPlayer()))
	{
		Controller->QuitGame();
	}
}
