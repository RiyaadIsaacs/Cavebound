#include "CaveboundBuildMenu.h"

#include "CaveboundCannon.h"
#include "CaveboundGameMode.h"
#include "CaveboundPlayerController.h"
#include "CaveboundPoisonDome.h"
#include "CaveboundSentry.h"
#include "CaveboundSnare.h"
#include "CaveboundTurret.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UCaveboundBuildChoiceProxy::HandleClicked()
{
	if (Menu)
	{
		Menu->SelectDefender(DefenderClass);
	}
}

void UCaveboundBuildMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BuildMenuCanvas"));
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Canvas;

	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BuildMenuFrame"));
	Frame->SetPadding(FMargin(16.f, 12.f));
	Frame->SetBrushColor(FLinearColor(0.08f, 0.06f, 0.04f, 0.94f));

	UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BuildMenuBody"));
	Frame->SetContent(Body);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BuildMenuTitle"));
	Title->SetText(FText::FromString(TEXT("Choose a defender")));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.9f, 0.75f, 1.f)));
	if (UVerticalBoxSlot* TitleSlot = Body->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	WoodText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BuildMenuWood"));
	WoodText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.74f, 0.55f, 1.f)));
	if (UVerticalBoxSlot* WoodSlot = Body->AddChildToVerticalBox(WoodText))
	{
		WoodSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	}

	ChoiceBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BuildMenuChoices"));
	Body->AddChildToVerticalBox(ChoiceBox);

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BuildMenuStatus"));
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.45f, 0.35f, 1.f)));
	if (UVerticalBoxSlot* StatusSlot = Body->AddChildToVerticalBox(StatusText))
	{
		StatusSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}

	PanelSlot = Canvas->AddChildToCanvas(Frame);
	if (PanelSlot)
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 1.f));
		PanelSlot->SetAutoSize(true);
	}
}

void UCaveboundBuildMenu::SetHoverScreenPosition(FVector2D Position)
{
	if (PanelSlot)
	{
		PanelSlot->SetPosition(Position);
	}
}

void UCaveboundBuildMenu::SetHoverVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void UCaveboundBuildMenu::OpenForSlot(AActor* BuildSlot)
{
	TargetSlot = BuildSlot;
	if (StatusText)
	{
		StatusText->SetText(FText::GetEmpty());
	}

	int32 Wood = 0;
	if (ACaveboundGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACaveboundGameMode>() : nullptr)
	{
		Wood = GameMode->GetWood();
	}
	if (WoodText)
	{
		WoodText->SetText(FText::FromString(FString::Printf(TEXT("Wood: %d"), Wood)));
	}

	if (ChoiceBox)
	{
		ChoiceBox->ClearChildren();
	}
	ChoiceProxies.Reset();

	if (UClass* TurretClass = LoadClass<AActor>(nullptr, TEXT("/Game/Blueprints/BP_Turret.BP_Turret_C")))
	{
		AddChoice(TEXT("Turret"), TurretClass);
	}

	// Blueprint versions hold the fire-point arrows placed in the editor.
	if (UClass* SentryClass = LoadClass<AActor>(nullptr, TEXT("/Game/Blueprints/BP_CaveboundSentry.BP_CaveboundSentry_C")))
	{
		AddChoice(TEXT("Sentry"), SentryClass);
	}
	else
	{
		AddChoice(TEXT("Sentry"), ACaveboundSentry::StaticClass());
	}

	if (UClass* CannonClass = LoadClass<AActor>(nullptr, TEXT("/Game/Blueprints/BP_CaveboundCannon.BP_CaveboundCannon_C")))
	{
		AddChoice(TEXT("Cannon"), CannonClass);
	}
	else
	{
		AddChoice(TEXT("Cannon"), ACaveboundCannon::StaticClass());
	}
	AddChoice(TEXT("Poison Dome"), ACaveboundPoisonDome::StaticClass());
	AddChoice(TEXT("Snare"), ACaveboundSnare::StaticClass());
}

void UCaveboundBuildMenu::AddChoice(const FString& Label, TSubclassOf<AActor> DefenderClass)
{
	if (!ChoiceBox || !DefenderClass || !WidgetTree)
	{
		return;
	}

	int32 Cost = 0;
	if (const ACaveboundTurret* Defaults = DefenderClass->GetDefaultObject<ACaveboundTurret>())
	{
		Cost = Defaults->GetCost();
	}

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	Button->SetBackgroundColor(FLinearColor(0.22f, 0.16f, 0.1f, 1.f));

	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(FText::FromString(FString::Printf(TEXT("%s    %d wood"), *Label, Cost)));
	LabelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	LabelText->SetJustification(ETextJustify::Center);
	Button->AddChild(LabelText);

	UCaveboundBuildChoiceProxy* Proxy = NewObject<UCaveboundBuildChoiceProxy>(this);
	Proxy->DefenderClass = DefenderClass;
	Proxy->Menu = this;
	ChoiceProxies.Add(Proxy);
	Button->OnClicked.AddDynamic(Proxy, &UCaveboundBuildChoiceProxy::HandleClicked);

	if (UVerticalBoxSlot* ButtonSlot = ChoiceBox->AddChildToVerticalBox(Button))
	{
		ButtonSlot->SetPadding(FMargin(0.f, 4.f));
	}
}

void UCaveboundBuildMenu::SelectDefender(TSubclassOf<AActor> DefenderClass)
{
	ACaveboundPlayerController* Controller = Cast<ACaveboundPlayerController>(GetOwningPlayer());
	AActor* BuildSlot = TargetSlot.Get();
	if (!Controller || !BuildSlot || !DefenderClass)
	{
		return;
	}

	if (!Controller->PlaceDefenderAtSlot(BuildSlot, DefenderClass))
	{
		if (StatusText)
		{
			StatusText->SetText(FText::FromString(TEXT("Not enough wood.")));
		}
		return;
	}

	Controller->CloseBuildMenu();
}
