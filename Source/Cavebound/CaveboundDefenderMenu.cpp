#include "CaveboundDefenderMenu.h"

#include "CaveboundGameMode.h"
#include "CaveboundPlayerController.h"
#include "CaveboundTurret.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Framework/Application/SlateApplication.h"

void UCaveboundDefenderActionProxy::HandleClicked()
{
	if (Menu)
	{
		Menu->HandleAction(Action);
	}
}

void UCaveboundDefenderMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DefenderMenuCanvas"));
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Canvas;

	FrameWidget = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DefenderMenuFrame"));
	FrameWidget->SetPadding(FMargin(16.f, 12.f));
	FrameWidget->SetBrushColor(FLinearColor(0.08f, 0.06f, 0.04f, 0.94f));

	UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DefenderMenuBody"));
	FrameWidget->SetContent(Body);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DefenderMenuTitle"));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.9f, 0.75f, 1.f)));
	if (UVerticalBoxSlot* TitleSlot = Body->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	ResourceText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DefenderMenuResources"));
	ResourceText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.74f, 0.55f, 1.f)));
	if (UVerticalBoxSlot* ResourceSlot = Body->AddChildToVerticalBox(ResourceText))
	{
		ResourceSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	ChoiceBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DefenderMenuChoices"));
	Body->AddChildToVerticalBox(ChoiceBox);

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DefenderMenuStatus"));
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.45f, 0.35f, 1.f)));
	if (UVerticalBoxSlot* StatusSlot = Body->AddChildToVerticalBox(StatusText))
	{
		StatusSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}

	PanelSlot = Canvas->AddChildToCanvas(FrameWidget);
	if (PanelSlot)
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 1.f));
		PanelSlot->SetAutoSize(true);
	}
}

void UCaveboundDefenderMenu::SetHoverScreenPosition(FVector2D Position)
{
	if (PanelSlot)
	{
		PanelSlot->SetPosition(Position);
	}
}

void UCaveboundDefenderMenu::SetHoverVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

bool UCaveboundDefenderMenu::IsPointerInsideMenu() const
{
	if (!FrameWidget || !FSlateApplication::IsInitialized())
	{
		return false;
	}

	return FrameWidget->GetCachedGeometry().IsUnderLocation(FSlateApplication::Get().GetCursorPos());
}

void UCaveboundDefenderMenu::OpenForDefender(AActor* BuildSlot, ACaveboundTurret* Defender)
{
	TargetSlot = BuildSlot;
	TargetDefender = Defender;

	if (StatusText)
	{
		StatusText->SetText(FText::GetEmpty());
	}

	int32 Wood = 0;
	int32 Rock = 0;
	if (ACaveboundGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACaveboundGameMode>() : nullptr)
	{
		Wood = GameMode->GetWood();
		Rock = GameMode->GetRock();
	}

	if (ResourceText)
	{
		ResourceText->SetText(FText::FromString(FString::Printf(TEXT("Wood: %d    Rock: %d"), Wood, Rock)));
	}

	const int32 level = Defender ? Defender->GetUpgradeLevel() : 0;
	if (TitleText)
	{
		const FString name = Defender ? Defender->GetHoverDisplayName_Implementation().ToString() : TEXT("Defender");
		TitleText->SetText(FText::FromString(FString::Printf(TEXT("%s    level %d"), *name, level + 1)));
	}

	if (ChoiceBox)
	{
		ChoiceBox->ClearChildren();
	}
	ActionProxies.Reset();

	if (!Defender)
	{
		return;
	}

	if (Defender->GetUpgradeLevel() >= Defender->GetMaxUpgradeLevel())
	{
		AddAction(TEXT("Upgrade"), TEXT("Max"), 0);
	}
	else if (Defender->GetNextUpgradeWoodCost() > 0)
	{
		AddAction(
			TEXT("Upgrade"),
			FString::Printf(TEXT("%d rock  %d wood"), Defender->GetNextUpgradeRockCost(), Defender->GetNextUpgradeWoodCost()),
			0);
	}
	else
	{
		AddAction(TEXT("Upgrade"), FString::Printf(TEXT("%d rock"), Defender->GetNextUpgradeRockCost()), 0);
	}

	if (Defender->GetRepairCost() <= 0)
	{
		AddAction(TEXT("Repair"), TEXT("Full"), 1);
	}
	else
	{
		AddAction(TEXT("Repair"), FString::Printf(TEXT("%d wood"), Defender->GetRepairCost()), 1);
	}

	int32 refundWood = 0;
	int32 refundRock = 0;
	Defender->GetDisarmRefund(refundWood, refundRock);
	AddAction(TEXT("Disarm"), FString::Printf(TEXT("+%d wood  +%d rock"), refundWood, refundRock), 2);
}

void UCaveboundDefenderMenu::AddAction(const FString& Label, const FString& Detail, int32 Action)
{
	if (!ChoiceBox || !WidgetTree)
	{
		return;
	}

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetBackgroundColor(FLinearColor(0.3f, 0.3f, 0.3f, 1.f));

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Button->AddChild(Row);

	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(FText::FromString(Label));
	LabelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText))
	{
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UTextBlock* DetailText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	DetailText->SetText(FText::FromString(Detail));
	DetailText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	DetailText->SetJustification(ETextJustify::Right);
	if (UHorizontalBoxSlot* DetailSlot = Row->AddChildToHorizontalBox(DetailText))
	{
		DetailSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
	}

	UCaveboundDefenderActionProxy* Proxy = NewObject<UCaveboundDefenderActionProxy>(this);
	Proxy->Menu = this;
	Proxy->Action = Action;
	ActionProxies.Add(Proxy);
	Button->OnClicked.AddDynamic(Proxy, &UCaveboundDefenderActionProxy::HandleClicked);

	if (UVerticalBoxSlot* ButtonSlot = ChoiceBox->AddChildToVerticalBox(Button))
	{
		ButtonSlot->SetPadding(FMargin(0.f, 4.f));
	}
}

void UCaveboundDefenderMenu::HandleAction(int32 Action)
{
	ACaveboundPlayerController* Controller = Cast<ACaveboundPlayerController>(GetOwningPlayer());
	AActor* BuildSlot = TargetSlot.Get();
	ACaveboundTurret* Defender = TargetDefender.Get();
	if (!Controller || !BuildSlot || !Defender)
	{
		return;
	}

	FString Failure;
	const bool bOk = Controller->UseDefenderAction(BuildSlot, Action, Failure);
	if (!bOk)
	{
		if (StatusText)
		{
			StatusText->SetText(FText::FromString(Failure));
		}
		return;
	}

	if (Action == 2 || !IsValid(Defender))
	{
		Controller->CloseDefenderMenu();
		return;
	}

	OpenForDefender(BuildSlot, Defender);
}
