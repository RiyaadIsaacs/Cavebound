#include "CaveboundBuildMenu.h"

#include "CaveboundCannon.h"
#include "CaveboundGameMode.h"
#include "CaveboundPlayerController.h"
#include "CaveboundPoisonDome.h"
#include "CaveboundSentry.h"
#include "CaveboundSnare.h"
#include "CaveboundTurret.h"
#include "CaveboundUIStyle.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Framework/Application/SlateApplication.h"

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

	SetVisibility(ESlateVisibility::Visible);

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BuildMenuCanvas"));
	Canvas->SetVisibility(ESlateVisibility::Visible);
	WidgetTree->RootWidget = Canvas;

	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BuildMenuFrame"));
	FrameWidget = Frame;
	Frame->SetPadding(FMargin(22.f, 18.f, 22.f, 20.f));
	Frame->SetBrush(CaveboundUIStyle::MakePanelBrush());

	UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BuildMenuBody"));
	Frame->SetContent(Body);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BuildMenuTitle"));
	Title->SetText(FText::FromString(TEXT("Choose a defender")));
	Title->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnWood()));
	Title->SetFont(CaveboundUIStyle::Font(18));
	Title->SetShadowColorAndOpacity(FLinearColor(0.12f, 0.08f, 0.06f, 0.5f));
	Title->SetShadowOffset(FVector2D(1.f, 1.f));
	if (UVerticalBoxSlot* TitleSlot = Body->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	WoodText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BuildMenuWood"));
	WoodText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::Body()));
	WoodText->SetFont(CaveboundUIStyle::Font(14));
	if (UVerticalBoxSlot* WoodSlot = Body->AddChildToVerticalBox(WoodText))
	{
		WoodSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	}

	ChoiceBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BuildMenuChoices"));
	Body->AddChildToVerticalBox(ChoiceBox);

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BuildMenuStatus"));
	StatusText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::Danger()));
	StatusText->SetFont(CaveboundUIStyle::Font(13));
	if (UVerticalBoxSlot* StatusSlot = Body->AddChildToVerticalBox(StatusText))
	{
		StatusSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}

	USizeBox* MenuSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BuildMenuSize"));
	MenuSize->SetMinDesiredWidth(320.f);
	MenuSize->SetContent(Frame);

	PanelSlot = Canvas->AddChildToCanvas(MenuSize);
	if (PanelSlot)
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 1.f));
		PanelSlot->SetAutoSize(true);
	}
}

void UCaveboundBuildMenu::SetHoverScreenPosition(FVector2D Position)
{
	if (!PanelSlot)
	{
		return;
	}

	// Alignment is bottom-center of the menu.
	FVector2D MenuSize = FrameWidget
		? FrameWidget->GetCachedGeometry().GetLocalSize()
		: FVector2D(320.f, 420.f);
	if (MenuSize.X < 1.f || MenuSize.Y < 1.f)
	{
		MenuSize = FVector2D(320.f, 420.f);
	}

	const float Scale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), 0.01f);
	const FVector2D Viewport = UWidgetLayoutLibrary::GetViewportSize(this) / Scale;
	const float Pad = 16.f;

	float X = Position.X;
	float Y = Position.Y;
	X = FMath::Clamp(X, Pad + MenuSize.X * 0.5f, Viewport.X - Pad - MenuSize.X * 0.5f);
	Y = FMath::Clamp(Y, Pad + MenuSize.Y, Viewport.Y - Pad);

	PanelSlot->SetPosition(FVector2D(X, Y));
}

void UCaveboundBuildMenu::SetHoverVisible(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (WidgetTree && WidgetTree->RootWidget)
	{
		WidgetTree->RootWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

bool UCaveboundBuildMenu::IsPointerInsideMenu() const
{
	if (!FrameWidget || !FSlateApplication::IsInitialized())
	{
		return false;
	}

	return FrameWidget->GetCachedGeometry().IsUnderLocation(FSlateApplication::Get().GetCursorPos());
}

void UCaveboundBuildMenu::OpenForSlot(AActor* BuildSlot)
{
	TargetSlot = BuildSlot;
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
	if (WoodText)
	{
		WoodText->SetText(FText::FromString(FString::Printf(TEXT("Wood: %d    Rock: %d"), Wood, Rock)));
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

	const int32 Cost = ACaveboundTurret::GetCostForClass(DefenderClass);

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	CaveboundUIStyle::ApplyRectButtonStyle(Button);

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Button->AddChild(Row);
	if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Button->GetContentSlot()))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
		ContentSlot->SetPadding(FMargin(4.f, 0.f));
	}

	UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	LabelText->SetText(FText::FromString(Label));
	LabelText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnCream()));
	LabelText->SetFont(CaveboundUIStyle::Font(13));
	LabelText->SetJustification(ETextJustify::Left);
	if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText))
	{
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LabelSlot->SetVerticalAlignment(VAlign_Center);
	}

	UTextBlock* CostText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	CostText->SetText(FText::FromString(FString::Printf(TEXT("%d wood"), Cost)));
	CostText->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnCream()));
	CostText->SetFont(CaveboundUIStyle::Font(12));
	CostText->SetJustification(ETextJustify::Right);
	if (UHorizontalBoxSlot* CostSlot = Row->AddChildToHorizontalBox(CostText))
	{
		CostSlot->SetHorizontalAlignment(HAlign_Right);
		CostSlot->SetVerticalAlignment(VAlign_Center);
		CostSlot->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
	}

	UCaveboundBuildChoiceProxy* Proxy = NewObject<UCaveboundBuildChoiceProxy>(this);
	Proxy->DefenderClass = DefenderClass;
	Proxy->Menu = this;
	ChoiceProxies.Add(Proxy);
	Button->OnClicked.AddDynamic(Proxy, &UCaveboundBuildChoiceProxy::HandleClicked);

	USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	ButtonSize->SetHeightOverride(72.f);
	ButtonSize->SetContent(Button);

	if (UVerticalBoxSlot* ButtonSlot = ChoiceBox->AddChildToVerticalBox(ButtonSize))
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
