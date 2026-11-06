#include "CaveboundPlayerController.h"
#include "CaveboundBuildMenu.h"
#include "CaveboundBuildPrompt.h"
#include "CaveboundCharacter.h"
#include "CaveboundControlsPanel.h"
#include "CaveboundDefenderMenu.h"
#include "CaveboundGameplayHud.h"
#include "CaveboundGameOverScreen.h"
#include "CaveboundPauseButton.h"
#include "CaveboundPauseMenu.h"
#include "CaveboundScreenFade.h"
#include "CaveboundTutorialCard.h"
#include "CaveboundWaveCountdown.h"
#include "CaveboundGameMode.h"
#include "CaveboundDamageVignette.h"
#include "CaveboundHoverHealth.h"
#include "CaveboundModelOutline.h"
#include "CaveboundEmptyCursor.h"
#include "CaveboundHoverInfo.h"
#include "CaveboundPlaceBeamVFX.h"
#include "CaveboundResourceReadout.h"
#include "CaveboundShaderMaterials.h"
#include "CaveboundUIStyle.h"
#include "Engine/PostProcessVolume.h"
#include "EngineUtils.h"
#include "CaveboundBaseEnemy.h"
#include "CaveboundRockDeposit.h"
#include "CaveboundTree.h"
#include "CaveboundTurret.h"
#include "Components/InputComponent.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/Widget.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/EditableText.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInterface.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

ACaveboundPlayerController::ACaveboundPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	DefaultMouseCursor = EMouseCursor::Default;
}

void ACaveboundPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ShowHUD();
	ClearEnhancedClickMappings();
	ApplyGameplayInputMode();

	bLeftMouseWasDown = IsMouseButtonHeld(EKeys::LeftMouseButton);
	bRightMouseWasDown = IsMouseButtonHeld(EKeys::RightMouseButton);

	if (ACaveboundGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACaveboundGameMode>() : nullptr)
	{
		GameMode->EnsureGameplayReady();
	}

	// fades the screen in when the match starts
	if (!ScreenFade)
	{
		ScreenFade = CreateWidget<UCaveboundScreenFade>(this, UCaveboundScreenFade::StaticClass());
		if (ScreenFade)
		{
			ScreenFade->AddToViewport(200);
			ScreenFade->StartFadeIn(1.6f);
		}
	}
}

void ACaveboundPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearEnhancedClickMappings();
	Super::EndPlay(EndPlayReason);
}

void ACaveboundPlayerController::ClearEnhancedClickMappings()
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->ClearAllMappings();
	}
}

void ACaveboundPlayerController::ShowHUD()
{
	// C++ HUD only — WBP_PlayerHUD is intentionally unused
	if (!HUDWidget)
	{
		HUDWidget = CreateWidget<UCaveboundGameplayHud>(this, UCaveboundGameplayHud::StaticClass());
		if (HUDWidget)
		{
			HUDWidget->AddToViewport(0);
			ConfigureHudClickThrough();
			CreateSkipCountdownButton();
			CreateSpeedButtons();
		}
	}

	if (!RockReadout)
	{
		RockReadout = CreateWidget<UCaveboundResourceReadout>(this, UCaveboundResourceReadout::StaticClass());
		if (RockReadout)
		{
			RockReadout->AddToViewport(1);
			RockReadout->Refresh();
		}
	}

	if (!WaveCountdown)
	{
		WaveCountdown = CreateWidget<UCaveboundWaveCountdown>(this, UCaveboundWaveCountdown::StaticClass());
		if (WaveCountdown)
		{
			WaveCountdown->AddToViewport(1);
			WaveCountdown->Refresh();
		}
	}

	if (!PauseButtonWidget)
	{
		PauseButtonWidget = CreateWidget<UCaveboundPauseButton>(this, UCaveboundPauseButton::StaticClass());
		if (PauseButtonWidget)
		{
			PauseButtonWidget->AddToViewport(2);
		}
	}

	// creates the right-side controls list
	if (!ControlsPanel)
	{
		ControlsPanel = CreateWidget<UCaveboundControlsPanel>(this, UCaveboundControlsPanel::StaticClass());
		if (ControlsPanel)
		{
			ControlsPanel->AddToViewport(1);
		}
	}

	// bottom-center tutorial card — locks movement until finished / skipped
	if (!TutorialCard)
	{
		TutorialCard = CreateWidget<UCaveboundTutorialCard>(this, UCaveboundTutorialCard::StaticClass());
		if (TutorialCard)
		{
			TutorialCard->AddToViewport(5);
			TutorialCard->BeginTutorial();
		}
	}

	// creates the hover health card overlay
	if (!HoverInfo)
	{
		HoverInfo = CreateWidget<UCaveboundHoverInfo>(this, UCaveboundHoverInfo::StaticClass());
		if (HoverInfo)
		{
			HoverInfo->AddToViewport(100);
		}
	}
	// creates the invisible software cursor widget
	if (!EmptyCursor)
	{
		EmptyCursor = CreateWidget<UCaveboundEmptyCursor>(this, UCaveboundEmptyCursor::StaticClass());
	}
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		// required so Custom/software cursors replace the OS arrow
		Viewport->SetUseSoftwareCursorWidgets(true);
		if (EmptyCursor)
		{
			Viewport->SetSoftwareCursorWidget(EMouseCursor::Custom, EmptyCursor);
		}
	}
	if (EmptyCursor)
	{
		SetMouseCursorWidget(EMouseCursor::Custom, EmptyCursor);
	}

	EnsureDamageVignette();

	// Clear leftover outline post-process volumes from earlier attempts.
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APostProcessVolume> It(World); It; ++It)
		{
			APostProcessVolume* Volume = *It;
			if (Volume && Volume->bUnbound && FMath::IsNearlyEqual(Volume->Priority, 50.f))
			{
				Volume->Destroy();
			}
		}
	}
}

void ACaveboundPlayerController::CreateSkipCountdownButton()
{
	if (!HUDWidget || !HUDWidget->WidgetTree || SkipCountdownButton)
	{
		return;
	}

	SkipCountdownCanvas = nullptr;
	if (UCaveboundGameplayHud* GameplayHud = Cast<UCaveboundGameplayHud>(HUDWidget))
	{
		SkipCountdownCanvas = GameplayHud->GetRootCanvas();
	}
	if (!SkipCountdownCanvas)
	{
		SkipCountdownCanvas = Cast<UCanvasPanel>(HUDWidget->WidgetTree->RootWidget);
	}
	if (!SkipCountdownCanvas)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateSkipCountdownButton: HUD has no canvas"));
		return;
	}

	// builds the New Wave button in C++ (WBP_PlayerHUD disabled)
	StartWaveButton = HUDWidget->WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("StartWaveButton"));
	CaveboundUIStyle::ApplyRectButtonStyle(StartWaveButton);

	UTextBlock* StartWaveLabel = HUDWidget->WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("StartWaveLabel"));
	StartWaveLabel->SetText(FText::FromString(TEXT("New Wave")));
	StartWaveLabel->SetJustification(ETextJustify::Center);
	StartWaveLabel->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnCream()));
	StartWaveLabel->SetFont(CaveboundUIStyle::Font(18));
	StartWaveButton->AddChild(StartWaveLabel);
	if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(StartWaveButton->GetContentSlot()))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Center);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
		ContentSlot->SetPadding(FMargin(4.f, 0.f));
	}
	StartWaveButton->OnClicked.AddDynamic(this, &ACaveboundPlayerController::RequestStartRound);

	if (UCanvasPanelSlot* StartSlot = SkipCountdownCanvas->AddChildToCanvas(StartWaveButton))
	{
		StartSlot->SetAnchors(FAnchors(1.f, 1.f));
		StartSlot->SetAlignment(FVector2D(1.f, 1.f));
		StartSlot->SetSize(FVector2D(240.f, 72.f));
		StartSlot->SetPosition(FVector2D(-24.f, -100.f));
		StartSlot->SetAutoSize(false);
	}

	SkipCountdownButton = HUDWidget->WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("SkipCountdownButton"));
	CaveboundUIStyle::ApplyRectButtonStyle(SkipCountdownButton);

	UTextBlock* Label = HUDWidget->WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("SkipCountdownLabel"));
	Label->SetText(FText::FromString(TEXT("Skip Countdown")));
	Label->SetJustification(ETextJustify::Center);
	Label->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnCream()));
	Label->SetFont(CaveboundUIStyle::Font(18));
	SkipCountdownButton->AddChild(Label);
	if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(SkipCountdownButton->GetContentSlot()))
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Center);
		ButtonSlot->SetVerticalAlignment(VAlign_Center);
		ButtonSlot->SetPadding(FMargin(4.f, 0.f));
	}

	SkipCountdownButtonSlot = SkipCountdownCanvas->AddChildToCanvas(SkipCountdownButton);
	if (SkipCountdownButtonSlot)
	{
		SkipCountdownButtonSlot->SetAnchors(FAnchors(0.f, 0.f));
		SkipCountdownButtonSlot->SetAlignment(FVector2D::ZeroVector);
		SkipCountdownButtonSlot->SetAutoSize(false);
		SkipCountdownButtonSlot->SetSize(FVector2D(240.f, 72.f));
	}

	SkipCountdownButton->OnClicked.AddDynamic(this, &ACaveboundPlayerController::HandleSkipCountdownClicked);
	SkipCountdownButton->SetVisibility(ESlateVisibility::Collapsed);
}

void ACaveboundPlayerController::UpdateSkipCountdownButton()
{
	if (!SkipCountdownButton || !SkipCountdownButtonSlot || !SkipCountdownCanvas)
	{
		return;
	}

	const ACaveboundGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ACaveboundGameMode>()
		: nullptr;
	if (!GameMode
		|| GameMode->GetRoundState() != ECaveboundRoundState::Collecting
		|| GameMode->GetCollectionTimeRemaining() <= 5.f)
	{
		SkipCountdownButton->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SkipCountdownButton->SetVisibility(ESlateVisibility::Visible);
	if (!StartWaveButton)
	{
		return;
	}

	const FGeometry StartGeometry = StartWaveButton->GetCachedGeometry();
	const FVector2D StartSize = StartGeometry.GetLocalSize();
	if (StartSize.IsNearlyZero())
	{
		return;
	}

	const FGeometry ViewportGeometry = SkipCountdownCanvas->GetCachedGeometry();
	FVector2D Position = ViewportGeometry.AbsoluteToLocal(StartGeometry.GetAbsolutePosition());
	const FVector2D SkipSize = StartSize;
	SkipCountdownButtonSlot->SetSize(SkipSize);
	SkipCountdownButtonSlot->SetPosition(Position);
}

void ACaveboundPlayerController::HandleSkipCountdownClicked()
{
	if (bPauseMenuOpen)
	{
		return;
	}

	if (ACaveboundGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ACaveboundGameMode>()
		: nullptr)
	{
		GameMode->SkipCollectionCountdown();
	}
}

void ACaveboundPlayerController::CreateSpeedButtons()
{
	if (!HUDWidget || !HUDWidget->WidgetTree || SpeedButton1)
	{
		return;
	}

	UCanvasPanel* Canvas = SkipCountdownCanvas;
	if (!Canvas)
	{
		Canvas = Cast<UCanvasPanel>(HUDWidget->WidgetTree->RootWidget);
	}
	if (!Canvas)
	{
		return;
	}

	auto MakeSpeedButton = [this](const FName Name, const FText& Label) -> UButton*
	{
		UButton* Button = HUDWidget->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		CaveboundUIStyle::ApplySquareButtonStyle(Button);

		UTextBlock* Text = HUDWidget->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(Label);
		Text->SetJustification(ETextJustify::Center);
		Text->SetColorAndOpacity(FSlateColor(CaveboundUIStyle::InkOnCream()));
		Text->SetFont(CaveboundUIStyle::Font(18));
		Button->AddChild(Text);
		if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Button->GetContentSlot()))
		{
			ContentSlot->SetHorizontalAlignment(HAlign_Center);
			ContentSlot->SetVerticalAlignment(VAlign_Center);
			ContentSlot->SetPadding(FMargin(0.f));
		}
		return Button;
	};

	UHorizontalBox* Row = HUDWidget->WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TEXT("SpeedButtonRow"));

	SpeedButton1 = MakeSpeedButton(TEXT("SpeedButton1"), FText::FromString(TEXT("x1")));
	SpeedButton2 = MakeSpeedButton(TEXT("SpeedButton2"), FText::FromString(TEXT("x2")));
	SpeedButton3 = MakeSpeedButton(TEXT("SpeedButton3"), FText::FromString(TEXT("x3")));

	auto AddToRow = [this, Row](UButton* Button)
	{
		USizeBox* Size = HUDWidget->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Size->SetWidthOverride(64.f);
		Size->SetHeightOverride(64.f);
		Size->SetContent(Button);
		if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Size))
		{
			Slot->SetPadding(FMargin(6.f, 0.f));
			Slot->SetVerticalAlignment(VAlign_Center);
		}
	};
	AddToRow(SpeedButton1);
	AddToRow(SpeedButton2);
	AddToRow(SpeedButton3);

	SpeedButton1->OnClicked.AddDynamic(this, &ACaveboundPlayerController::HandleSpeed1Clicked);
	SpeedButton2->OnClicked.AddDynamic(this, &ACaveboundPlayerController::HandleSpeed2Clicked);
	SpeedButton3->OnClicked.AddDynamic(this, &ACaveboundPlayerController::HandleSpeed3Clicked);

	SpeedButtonsSlot = Canvas->AddChildToCanvas(Row);
	if (SpeedButtonsSlot)
	{
		// Y stays bottom-right; X is synced to the New Wave button in AlignRightSideUI
		SpeedButtonsSlot->SetAnchors(FAnchors(1.f, 1.f));
		SpeedButtonsSlot->SetAlignment(FVector2D(1.f, 1.f));
		SpeedButtonsSlot->SetPosition(FVector2D(-24.f, -24.f));
		SpeedButtonsSlot->SetAutoSize(true);
	}

	SetGameSpeed(1.f);
}

void ACaveboundPlayerController::AlignRightSideUI()
{
	if (!StartWaveButton || !SkipCountdownCanvas || !SpeedButtonsSlot)
	{
		return;
	}

	UCanvasPanelSlot* StartSlot = Cast<UCanvasPanelSlot>(StartWaveButton->Slot);
	if (!StartSlot)
	{
		return;
	}

	const FGeometry CanvasGeometry = SkipCountdownCanvas->GetCachedGeometry();
	const FVector2D CanvasSize = CanvasGeometry.GetLocalSize();
	if (CanvasSize.X < 1.f || CanvasSize.Y < 1.f)
	{
		return;
	}

	// same gap used between speed buttons (6px padding on each side)
	constexpr float EdgePad = 24.f;
	constexpr float ClusterGap = 12.f;
	constexpr float SpeedHeight = 64.f;

	FVector2D StartSize = StartSlot->GetSize();
	if (StartSize.X < 1.f || StartSize.Y < 1.f)
	{
		StartSize = FVector2D(240.f, 72.f);
	}

	const float RightOffset = -EdgePad;

	// speed row sits on the bottom-right edge
	SpeedButtonsSlot->SetAnchors(FAnchors(1.f, 1.f));
	SpeedButtonsSlot->SetAlignment(FVector2D(1.f, 1.f));
	SpeedButtonsSlot->SetPosition(FVector2D(RightOffset, -EdgePad));
	SpeedButtonsSlot->SetAutoSize(true);

	// lifts New Wave above the speed row by the same cluster gap
	StartSlot->SetAnchors(FAnchors(1.f, 1.f));
	StartSlot->SetAlignment(FVector2D(1.f, 1.f));
	StartSlot->SetAutoSize(false);
	StartSlot->SetSize(StartSize);
	StartSlot->SetPosition(FVector2D(RightOffset, -(EdgePad + SpeedHeight + ClusterGap)));

	const FGeometry StartGeometry = StartWaveButton->GetCachedGeometry();
	const FVector2D StartAbsPos = StartGeometry.GetAbsolutePosition();
	const FVector2D StartAbsSize = StartGeometry.GetAbsoluteSize();
	if (StartAbsSize.IsNearlyZero())
	{
		return;
	}

	const float StartRightAbs = StartAbsPos.X + StartAbsSize.X;
	if (ControlsPanel)
	{
		ControlsPanel->SetMinWidth(StartAbsSize.X);
		ControlsPanel->AlignRightEdgeToAbsolute(StartRightAbs);
	}
}

void ACaveboundPlayerController::AlignTopHud()
{
	if (!RockReadout || !PauseButtonWidget)
	{
		return;
	}

	FVector2D TreeCenter;
	if (RockReadout->GetTreeBarCenterAbsolute(TreeCenter))
	{
		PauseButtonWidget->AlignToTreeBarCenter(TreeCenter);
	}
}

void ACaveboundPlayerController::ToggleControlsPanel()
{
	if (bPauseMenuOpen || !ControlsPanel)
	{
		return;
	}

	ControlsPanel->ToggleCollapsed();
}

void ACaveboundPlayerController::SetGameSpeed(float Speed)
{
	GameSpeed = Speed;
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, Speed);
	}
	RefreshSpeedButtonStyles();
}

void ACaveboundPlayerController::RefreshSpeedButtonStyles()
{
	auto StyleButton = [](UButton* Button, bool bActive)
	{
		if (!Button)
		{
			return;
		}
		CaveboundUIStyle::ApplySquareButtonStyle(Button);
		// Active speed reads a bit brighter.
		Button->SetColorAndOpacity(bActive ? FLinearColor(1.15f, 1.1f, 0.95f, 1.f) : FLinearColor::White);
	};

	StyleButton(SpeedButton1, FMath::IsNearlyEqual(GameSpeed, 1.f));
	StyleButton(SpeedButton2, FMath::IsNearlyEqual(GameSpeed, 2.f));
	StyleButton(SpeedButton3, FMath::IsNearlyEqual(GameSpeed, 3.f));
}

void ACaveboundPlayerController::HandleSpeed1Clicked()
{
	SetGameSpeed(1.f);
}

void ACaveboundPlayerController::HandleSpeed2Clicked()
{
	SetGameSpeed(2.f);
}

void ACaveboundPlayerController::HandleSpeed3Clicked()
{
	SetGameSpeed(3.f);
}

void ACaveboundPlayerController::ConfigureHudClickThrough()
{
	if (!HUDWidget)
	{
		return;
	}

	HUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (!HUDWidget->WidgetTree)
	{
		return;
	}

	HUDWidget->WidgetTree->ForEachWidget([](UWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}

		// Leave interactive + progress widgets alone so HUD health bars still show/toggle.
		if (Widget->IsA<UButton>()
			|| Widget->IsA<UCheckBox>()
			|| Widget->IsA<USlider>()
			|| Widget->IsA<UEditableText>()
			|| Widget->IsA<UEditableTextBox>()
			|| Widget->IsA<UProgressBar>())
		{
			return;
		}

		if (Widget->GetVisibility() == ESlateVisibility::Visible)
		{
			Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	});
}

void ACaveboundPlayerController::ApplyGameplayInputMode()
{
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;

	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
		Viewport->SetHideCursorDuringCapture(false);
		Viewport->SetMouseLockMode(EMouseLockMode::DoNotLock);
	}

	ClearEnhancedClickMappings();
	ConfigureHudClickThrough();
}

void ACaveboundPlayerController::ApplyMenuInputMode()
{
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;

	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
		Viewport->SetHideCursorDuringCapture(false);
	}
}

void ACaveboundPlayerController::ApplyPauseInputMode()
{
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (bGameOverScreenOpen && GameOverScreen)
	{
		InputMode.SetWidgetToFocus(GameOverScreen->TakeWidget());
	}
	else if (PauseMenuWidget)
	{
		InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	}
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ACaveboundPlayerController::ShowPauseMenu()
{
	// C++ pause menu only — WBP_PauseMenu is intentionally unused
	if (!PauseMenuWidget)
	{
		PauseMenuWidget = CreateWidget<UCaveboundPauseMenu>(this, UCaveboundPauseMenu::StaticClass());
	}

	if (PauseMenuWidget && !PauseMenuWidget->IsInViewport())
	{
		PauseMenuWidget->AddToViewport(10);
	}
}

void ACaveboundPlayerController::HidePauseMenu()
{
	if (PauseMenuWidget && PauseMenuWidget->IsInViewport())
	{
		PauseMenuWidget->RemoveFromParent();
	}
}

void ACaveboundPlayerController::TogglePauseMenu()
{
	if (bGameOverScreenOpen)
	{
		return;
	}

	if (BuildMenu && BuildMenu->IsInViewport())
	{
		CloseBuildMenu();
		return;
	}

	if (bPauseMenuOpen)
	{
		ResumeGame();
	}
	else
	{
		PauseGame();
	}
}

void ACaveboundPlayerController::PauseGame()
{
	if (bPauseMenuOpen || bGameOverScreenOpen)
	{
		return;
	}

	bPauseMenuOpen = true;
	bOrbitingCamera = false;
	UGameplayStatics::SetGamePaused(this, true);
	ShowPauseMenu();
	ApplyPauseInputMode();
}

void ACaveboundPlayerController::ResumeGame()
{
	if (!bPauseMenuOpen)
	{
		return;
	}

	bPauseMenuOpen = false;
	HidePauseMenu();
	UGameplayStatics::SetGamePaused(this, false);
	SetGameSpeed(GameSpeed);
	ApplyGameplayInputMode();
}

void ACaveboundPlayerController::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void ACaveboundPlayerController::RestartMatch()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UGameplayStatics::SetGamePaused(this, false);
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(World, true);
	UGameplayStatics::OpenLevel(World, FName(*LevelName));
}

void ACaveboundPlayerController::ShowGameOverScreen()
{
	if (bGameOverScreenOpen)
	{
		return;
	}

	bGameOverScreenOpen = true;
	bPauseMenuOpen = false;
	HidePauseMenu();
	CloseBuildMenu();
	CloseDefenderMenu();

	if (!GameOverScreen)
	{
		GameOverScreen = CreateWidget<UCaveboundGameOverScreen>(this, UCaveboundGameOverScreen::StaticClass());
	}
	if (GameOverScreen && !GameOverScreen->IsInViewport())
	{
		GameOverScreen->AddToViewport(20);
	}

	UGameplayStatics::SetGamePaused(this, true);
	ApplyPauseInputMode();
}

bool ACaveboundPlayerController::IsInGracePeriod() const
{
	const ACaveboundGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<ACaveboundGameMode>()
		: nullptr;
	return GameMode && GameMode->IsRoundIdle();
}

void ACaveboundPlayerController::NotifyGracePhaseBlocked()
{
	if (RockReadout)
	{
		RockReadout->ShowGracePhaseTip();
	}
}

void ACaveboundPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		// Escape pauses / resumes even while the world is paused
		FInputKeyBinding& EscapeBinding = InputComponent->BindKey(
			EKeys::Escape,
			IE_Pressed,
			this,
			&ACaveboundPlayerController::TogglePauseMenu);
		EscapeBinding.bExecuteWhenPaused = true;

		// Tab collapses / expands the controls card
		InputComponent->BindKey(
			EKeys::Tab,
			IE_Pressed,
			this,
			&ACaveboundPlayerController::ToggleControlsPanel);

		InputComponent->BindKey(EKeys::B, IE_Pressed, this, &ACaveboundPlayerController::HandleBuildKey);
		InputComponent->BindKey(EKeys::F, IE_Pressed, this, &ACaveboundPlayerController::HandleDefenderKey);
		InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ACaveboundPlayerController::RequestStartRound);
		InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ACaveboundPlayerController::AdvanceTutorial);
		InputComponent->BindKey(EKeys::H, IE_Pressed, this, &ACaveboundPlayerController::SkipTutorial);
	}
}

bool ACaveboundPlayerController::IsTutorialActive() const
{
	return TutorialCard && TutorialCard->IsActive();
}

void ACaveboundPlayerController::AdvanceTutorial()
{
	if (bPauseMenuOpen || bGameOverScreenOpen || !TutorialCard)
	{
		return;
	}

	TutorialCard->Advance();
}

void ACaveboundPlayerController::SkipTutorial()
{
	if (bPauseMenuOpen || bGameOverScreenOpen || !TutorialCard)
	{
		return;
	}

	TutorialCard->Skip();
}

void ACaveboundPlayerController::RequestStartRound()
{
	if (bPauseMenuOpen || bGameOverScreenOpen || IsTutorialActive())
	{
		return;
	}

	ACaveboundGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACaveboundGameMode>() : nullptr;
	UE_LOG(
		LogTemp,
		Log,
		TEXT("RequestStartRound: GameMode=%s"),
		GameMode ? *GameMode->GetName() : TEXT("null"));
	if (GameMode)
	{
		GameMode->StartRound();
	}
}

void ACaveboundPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	StripSlotPlaceKeys();
	UpdateBuildSlotPresentation();
	if (ScreenFade)
	{
		ScreenFade->TickFade(DeltaTime);
	}
	if (RockReadout)
	{
		RockReadout->Refresh();
		RockReadout->UpdateGracePhaseTip();
	}
	if (WaveCountdown)
	{
		WaveCountdown->Refresh();
	}
	UpdateSkipCountdownButton();
	AlignRightSideUI();
	AlignTopHud();

	// presents game over once the tree is dead
	if (!bGameOverScreenOpen)
	{
		if (const ACaveboundGameMode* GameMode = GetWorld()
			? GetWorld()->GetAuthGameMode<ACaveboundGameMode>()
			: nullptr)
		{
			if (GameMode->IsGameOver())
			{
				ShowGameOverScreen();
			}
		}
	}

	if (DamageVignette)
	{
		DamageVignette->TickFade(DeltaTime);
	}
	SyncTreeScreenDarken();

	if (bPauseMenuOpen)
	{
		if (HoverInfo)
		{
			HoverInfo->HideInfo();
		}
		ClearHoverGlow();
		SetHoverInfoAsCursor(false);
		bLeftMouseWasDown = IsMouseButtonHeld(EKeys::LeftMouseButton);
		bRightMouseWasDown = IsMouseButtonHeld(EKeys::RightMouseButton);
		return;
	}

	const bool bLMB = IsMouseButtonHeld(EKeys::LeftMouseButton);
	const bool bRMB = IsMouseButtonHeld(EKeys::RightMouseButton);

	float MouseX = 0.f;
	float MouseY = 0.f;
	const bool bMouseInViewport = GetMousePosition(MouseX, MouseY);

	if (IsTutorialActive())
	{
		// movement and camera locked while the tutorial card is up
		bLeftMouseWasDown = bLMB;
		bRightMouseWasDown = bRMB;
		bOrbitingCamera = false;
		return;
	}

	if (bLMB && !bLeftMouseWasDown && bMouseInViewport)
	{
		if (BuildMenu && BuildMenu->IsInViewport() && !BuildMenu->IsPointerInsideMenu())
		{
			CloseBuildMenu();
		}
		if (DefenderMenu && DefenderMenu->IsInViewport() && !DefenderMenu->IsPointerInsideMenu())
		{
			CloseDefenderMenu();
		}
		OnClickMove();
	}
	bLeftMouseWasDown = bLMB;

	if (bRMB && bMouseInViewport)
	{
		if (!bRightMouseWasDown)
		{
			LastOrbitMouseX = MouseX;
			LastOrbitMouseY = MouseY;
		}
		else
		{
			const float DeltaX = MouseX - LastOrbitMouseX;
			const float DeltaY = MouseY - LastOrbitMouseY;
			if (!FMath::IsNearlyZero(DeltaX) || !FMath::IsNearlyZero(DeltaY))
			{
				if (ACaveboundCharacter* PlayerCharacter = Cast<ACaveboundCharacter>(GetPawn()))
				{
					PlayerCharacter->OrbitCamera(DeltaX, DeltaY);
				}
			}
			LastOrbitMouseX = MouseX;
			LastOrbitMouseY = MouseY;
		}
		bOrbitingCamera = true;
	}
	else
	{
		bOrbitingCamera = false;
	}
	bRightMouseWasDown = bRMB;

	UpdateHoveredHealthTarget();
	UpdateHoverInfoWidget();
	UpdateHoverGlow();
}

bool ACaveboundPlayerController::IsMouseButtonHeld(const FKey& Button) const
{
#if PLATFORM_WINDOWS
	if (Button == EKeys::LeftMouseButton)
	{
		return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
	}
	if (Button == EKeys::RightMouseButton)
	{
		return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
	}
#endif

	if (FSlateApplication::IsInitialized())
	{
		return FSlateApplication::Get().GetPressedMouseButtons().Contains(Button);
	}

	return IsInputKeyDown(Button);
}

bool ACaveboundPlayerController::IsPointerOverInteractiveUI() const
{
	if (bPauseMenuOpen || bGameOverScreenOpen)
	{
		return true;
	}

	auto HasHoveredInteractiveChild = [](const UUserWidget* Root) -> bool
	{
		if (!Root || !Root->WidgetTree)
		{
			return false;
		}

		bool bFound = false;
		Root->WidgetTree->ForEachWidget([&bFound](UWidget* Widget)
		{
			if (bFound || !Widget || !Widget->IsHovered())
			{
				return;
			}

			if (Widget->IsA<UButton>()
				|| Widget->IsA<UCheckBox>()
				|| Widget->IsA<USlider>()
				|| Widget->IsA<UEditableText>()
				|| Widget->IsA<UEditableTextBox>())
			{
				bFound = true;
			}
		});
		return bFound;
	};

	if (HasHoveredInteractiveChild(HUDWidget))
	{
		return true;
	}

	if (BuildMenu && BuildMenu->IsInViewport() && HasHoveredInteractiveChild(BuildMenu))
	{
		return true;
	}

	if (SkipCountdownButton && SkipCountdownButton->IsHovered())
	{
		return true;
	}
	if ((SpeedButton1 && SpeedButton1->IsHovered())
		|| (SpeedButton2 && SpeedButton2->IsHovered())
		|| (SpeedButton3 && SpeedButton3->IsHovered()))
	{
		return true;
	}

	if (PauseButtonWidget && HasHoveredInteractiveChild(PauseButtonWidget))
	{
		return true;
	}

	return false;
}

void ACaveboundPlayerController::OnClickMove()
{
	if (bPauseMenuOpen
		|| IsTutorialActive()
		|| (BuildMenu && BuildMenu->IsInViewport())
		|| (DefenderMenu && DefenderMenu->IsInViewport())
		|| IsPointerOverInteractiveUI())
	{
		return;
	}

	UWorld* World = GetWorld();
	APawn* ControlledPawn = GetPawn();
	if (!World || !ControlledPawn)
	{
		return;
	}

	FVector WorldLocation;
	FVector WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		return;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ClickMove), false, ControlledPawn);
	const FVector TraceEnd = WorldLocation + WorldDirection * 100000.f;
	if (!World->LineTraceSingleByChannel(Hit, WorldLocation, TraceEnd, ECC_Visibility, Params))
	{
		return;
	}

	if (ACaveboundCharacter* PlayerCharacter = Cast<ACaveboundCharacter>(ControlledPawn))
	{
		if (ACaveboundTree* Tree = Cast<ACaveboundTree>(Hit.GetActor()))
		{
			PlayerCharacter->SetTreeTarget(Tree);
		}
		else if (ACaveboundRockDeposit* Rock = Cast<ACaveboundRockDeposit>(Hit.GetActor()))
		{
			PlayerCharacter->SetRockTarget(Rock);
		}
		else
		{
			PlayerCharacter->SetMoveDestination(Hit.ImpactPoint);
		}
	}
}

AActor* ACaveboundPlayerController::TraceActorUnderCursor() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// deprojects mouse into a world ray
	FVector WorldLocation;
	FVector WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		return nullptr;
	}

	// complex mesh hits so big collision boxes don't stick hover
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CursorTrace), /*bTraceComplex*/ true, GetPawn());
	const FVector TraceEnd = WorldLocation + WorldDirection * 100000.f;

	TArray<FHitResult> Hits;
	World->LineTraceMultiByChannel(Hits, WorldLocation, TraceEnd, ECC_Visibility, Params);
	if (Hits.Num() == 0)
	{
		World->LineTraceMultiByChannel(Hits, WorldLocation, TraceEnd, ECC_WorldDynamic, Params);
	}

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!IsValid(HitActor) || HitActor == GetPawn())
		{
			continue;
		}
		return HitActor;
	}
	return nullptr;
}

static AActor* FindHoverHealthActor(AActor* Start)
{
	AActor* Candidate = Start;
	while (Candidate)
	{
		if (Candidate->Implements<UCaveboundHoverHealth>())
		{
			return Candidate;
		}
		AActor* Next = Candidate->GetAttachParentActor();
		if (!Next)
		{
			Next = Candidate->GetOwner();
		}
		if (Next == Candidate)
		{
			break;
		}
		Candidate = Next;
	}
	return nullptr;
}

bool ACaveboundPlayerController::IsCursorOverActorVisual(const AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	float MouseX = 0.f;
	float MouseY = 0.f;
	if (!GetMousePosition(MouseX, MouseY))
	{
		return false;
	}
	const FVector2D Mouse(MouseX, MouseY);

	FVector Origin = Actor->GetActorLocation();
	FVector Extent(35.f, 35.f, 70.f);

	FBox Combined(ForceInit);
	bool bGotBounds = false;
	TArray<UMeshComponent*> Meshes;
	Actor->GetComponents<UMeshComponent>(Meshes);
	for (const UMeshComponent* Mesh : Meshes)
	{
		if (!Mesh || !Mesh->IsRegistered() || Mesh->bHiddenInGame || !Mesh->IsVisible())
		{
			continue;
		}
		if (const UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Mesh))
		{
			if (!StaticMesh->GetStaticMesh())
			{
				continue;
			}
		}
		Combined += Mesh->Bounds.GetBox();
		bGotBounds = true;
	}

	if (bGotBounds)
	{
		Origin = Combined.GetCenter();
		Extent = Combined.GetExtent();
	}
	else
	{
		Actor->GetActorBounds(true, Origin, Extent);
	}

	Extent.X = FMath::Min(Extent.X, 90.f);
	Extent.Y = FMath::Min(Extent.Y, 90.f);
	Extent.Z = FMath::Min(Extent.Z, 160.f);

	FVector2D MinScreen(FLT_MAX, FLT_MAX);
	FVector2D MaxScreen(-FLT_MAX, -FLT_MAX);
	int32 Projected = 0;
	for (int32 IX = -1; IX <= 1; IX += 2)
	{
		for (int32 IY = -1; IY <= 1; IY += 2)
		{
			for (int32 IZ = -1; IZ <= 1; IZ += 2)
			{
				const FVector Corner = Origin + FVector(Extent.X * IX, Extent.Y * IY, Extent.Z * IZ);
				FVector2D Screen;
				if (ProjectWorldLocationToScreen(Corner, Screen, true))
				{
					MinScreen.X = FMath::Min(MinScreen.X, Screen.X);
					MinScreen.Y = FMath::Min(MinScreen.Y, Screen.Y);
					MaxScreen.X = FMath::Max(MaxScreen.X, Screen.X);
					MaxScreen.Y = FMath::Max(MaxScreen.Y, Screen.Y);
					++Projected;
				}
			}
		}
	}

	if (Projected < 2)
	{
		return false;
	}

	const float Pad = 4.f;
	MinScreen.X -= Pad;
	MinScreen.Y -= Pad;
	MaxScreen.X += Pad;
	MaxScreen.Y += Pad;

	const FVector2D Center = (MinScreen + MaxScreen) * 0.5f;
	FVector2D Half = (MaxScreen - MinScreen) * 0.5f;
	Half.X = FMath::Clamp(Half.X, 14.f, 70.f);
	Half.Y = FMath::Clamp(Half.Y, 14.f, 100.f);

	return Mouse.X >= Center.X - Half.X
		&& Mouse.X <= Center.X + Half.X
		&& Mouse.Y >= Center.Y - Half.Y
		&& Mouse.Y <= Center.Y + Half.Y;
}

bool ACaveboundPlayerController::IsBuildSlotActor(const AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	static TWeakObjectPtr<UClass> SlotClass;
	if (!SlotClass.IsValid())
	{
		SlotClass = LoadClass<AActor>(nullptr, TEXT("/Game/Blueprints/BP_BuildSlot.BP_BuildSlot_C"));
	}
	return SlotClass.IsValid() && Actor->IsA(SlotClass.Get());
}

// picks glow tint by hovered actor type
FLinearColor ACaveboundPlayerController::GetHoverGlowColor(const AActor* Actor) const
{
	if (!Actor)
	{
		return FLinearColor::White;
	}
	if (Actor->IsA(ACaveboundTree::StaticClass()))
	{
		return FLinearColor(0.35f, 2.2f, 0.55f); // green
	}
	if (Actor->IsA(ACaveboundTurret::StaticClass()))
	{
		return FLinearColor(0.35f, 1.1f, 2.4f); // blue
	}
	if (Actor->IsA(ACaveboundRockDeposit::StaticClass()))
	{
		return FLinearColor(1.35f, 1.35f, 1.4f); // grey
	}
	if (IsBuildSlotActor(Actor))
	{
		return FLinearColor(2.f, 2.f, 2.f); // white
	}
	return FLinearColor(1.8f, 1.8f, 1.8f);
}

// removes hover glow and restores outline / pad highlight
void ACaveboundPlayerController::ClearHoverGlow()
{
	AActor* Prev = HoverGlowActor.Get();
	HoverGlowActor = nullptr;
	if (!IsValid(Prev))
	{
		return;
	}

	// nearby empty slots keep soft white pad highlight; else restore outline
	const bool bStillNearby = HighlightedSlots.ContainsByPredicate(
		[Prev](const TWeakObjectPtr<AActor>& Existing)
		{
			return Existing.Get() == Prev;
		});
	if (bStillNearby)
	{
		SetSlotHighlighted(Prev, true);
	}
	else
	{
		CaveboundModelOutline::RestoreActor(Prev);
	}
}

// applies colored overlay glow to the hovered actor's meshes
void ACaveboundPlayerController::ApplyHoverGlow(AActor* Actor, const FLinearColor& Color)
{
	if (!IsValid(Actor))
	{
		return;
	}

	// rocks use flat fill; enemies get hover info only (no glow)
	const bool bRock = Actor->IsA(ACaveboundRockDeposit::StaticClass());

	UMaterialInstanceDynamic* GlowMID = nullptr;
	if (bRock)
	{
		// flat translucent fill for rocks
		if (!HoverGlowFillMaterial)
		{
			if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(
				nullptr,
				TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent")))
			{
				HoverGlowFillMaterial = UMaterialInstanceDynamic::Create(Base, this);
			}
		}
		GlowMID = HoverGlowFillMaterial;
		if (GlowMID)
		{
			GlowMID->SetVectorParameterValue(
				FName(TEXT("Color")),
				FLinearColor(0.75f, 0.75f, 0.78f, 0.6f));
		}
	}
	else
	{
		// outline glow MID for trees / towers / pads
		if (!HoverGlowMaterial)
		{
			HoverGlowMaterial = CaveboundShaderMaterials::CreateHoverOutlineMID(this);
			if (!HoverGlowMaterial)
			{
				if (UMaterialInterface* Fallback = LoadObject<UMaterialInterface>(
					nullptr,
					TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent")))
				{
					HoverGlowMaterial = UMaterialInstanceDynamic::Create(Fallback, this);
				}
			}
		}
		GlowMID = HoverGlowMaterial;
		if (GlowMID)
		{
			const bool bBuildSlot = IsBuildSlotActor(Actor);
			GlowMID->SetVectorParameterValue(TEXT("OutlineColor"), Color);
			GlowMID->SetVectorParameterValue(TEXT("Color"), Color);
			GlowMID->SetScalarParameterValue(TEXT("Intensity"), bBuildSlot ? 4.5f : 6.f);
			GlowMID->SetScalarParameterValue(TEXT("OpacityBoost"), bBuildSlot ? 2.2f : 3.f);
			GlowMID->SetScalarParameterValue(TEXT("Exponent"), bBuildSlot ? 2.f : 2.3f);
			GlowMID->SetScalarParameterValue(TEXT("MinOpacity"), bBuildSlot ? 0.45f : 0.22f);
		}
	}

	if (!GlowMID)
	{
		return;
	}

	// paints the glow onto every visible mesh
	TArray<UMeshComponent*> Meshes;
	Actor->GetComponents<UMeshComponent>(Meshes);
	for (UMeshComponent* Mesh : Meshes)
	{
		if (!IsValid(Mesh) || Mesh->bHiddenInGame || !Mesh->GetVisibleFlag())
		{
			continue;
		}
		if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Mesh))
		{
			if (!StaticMesh->GetStaticMesh())
			{
				continue;
			}
			StaticMesh->bDisallowNanite = true;
		}
		Mesh->SetOverlayMaterial(GlowMID);
	}

	HoverGlowActor = Actor;
}

// keeps glow on the current hover target (skips enemies)
void ACaveboundPlayerController::UpdateHoverGlow()
{
	AActor* Target = HoveredHealthActor.Get();
	// enemies keep hover info, but no glow
	if (Target && Target->IsA(ACaveboundBaseEnemy::StaticClass()))
	{
		Target = nullptr;
	}
	if (!IsValid(Target))
	{
		Target = HoveredBuildSlot.Get();
	}
	if (!IsValid(Target))
	{
		ClearHoverGlow();
		return;
	}

	const FLinearColor Color = GetHoverGlowColor(Target);
	if (HoverGlowActor.Get() != Target)
	{
		ClearHoverGlow();
		ApplyHoverGlow(Target, Color);
		return;
	}

	// reapply so a damage flash clearing the overlay doesn't stick
	ApplyHoverGlow(Target, Color);
}

// traces under cursor to find hover health / build slot targets
void ACaveboundPlayerController::UpdateHoveredHealthTarget()
{
	HoveredHealthActor = nullptr;
	HoveredBuildSlot = nullptr;

	if (bOrbitingCamera || bPauseMenuOpen || IsPointerOverInteractiveUI())
	{
		return;
	}

	// skip while a build / defender menu is open
	if ((BuildMenu && BuildMenu->IsInViewport())
		|| (DefenderMenu && DefenderMenu->IsInViewport()))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector WorldLocation;
	FVector WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		return;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(HoverHealthTrace), /*bTraceComplex*/ true, GetPawn());
	const FVector TraceEnd = WorldLocation + WorldDirection * 100000.f;

	// gathers visibility + dynamic hits, nearest first
	TArray<FHitResult> Hits;
	World->LineTraceMultiByChannel(Hits, WorldLocation, TraceEnd, ECC_Visibility, Params);
	{
		TArray<FHitResult> DynamicHits;
		World->LineTraceMultiByChannel(DynamicHits, WorldLocation, TraceEnd, ECC_WorldDynamic, Params);
		Hits.Append(DynamicHits);
	}

	Hits.Sort([](const FHitResult& A, const FHitResult& B)
	{
		return A.Distance < B.Distance;
	});

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!IsValid(HitActor) || HitActor == GetPawn())
		{
			continue;
		}

		if (AActor* HoverActor = FindHoverHealthActor(HitActor))
		{
			// gate on visible silhouette (enemies use invisible hit volumes)
			if (IsCursorOverActorVisual(HoverActor))
			{
				HoveredHealthActor = HoverActor;
				return;
			}
			continue;
		}

		if (IsBuildSlotActor(HitActor) && IsCursorOverActorVisual(HitActor))
		{
			// occupied pads prefer the defender for hover
			if (ACaveboundTurret* Defender = GetDefenderOnSlot(HitActor))
			{
				if (!Defender->IsDestroyed())
				{
					HoveredHealthActor = Defender;
					return;
				}
			}
			HoveredBuildSlot = HitActor;
			return;
		}
	}

	// fallback: nearby empty pads often miss visibility traces
	if (!HoveredHealthActor.IsValid() && !HoveredBuildSlot.IsValid())
	{
		TArray<AActor*> NearbySlots;
		AActor* Closest = nullptr;
		CollectInteractableSlots(NearbySlots, Closest);

		float MouseX = 0.f;
		float MouseY = 0.f;
		if (GetMousePosition(MouseX, MouseY))
		{
			const FVector2D Mouse(MouseX, MouseY);
			AActor* BestSlot = nullptr;
			float BestDistSq = TNumericLimits<float>::Max();
			for (AActor* Slot : NearbySlots)
			{
				// CollectInteractableSlots already skips occupied, but guard anyway.
				if (ACaveboundTurret* Defender = GetDefenderOnSlot(Slot))
				{
					if (!Defender->IsDestroyed())
					{
						continue;
					}
				}
				if (!IsCursorOverActorVisual(Slot))
				{
					continue;
				}
				FVector2D PixelPos;
				const FVector WorldPos = Slot->GetActorLocation() + FVector(0.f, 0.f, 40.f);
				if (!ProjectWorldLocationToScreen(WorldPos, PixelPos, true))
				{
					continue;
				}
				const float DistSq = FVector2D::DistSquared(PixelPos, Mouse);
				if (DistSq < BestDistSq)
				{
					BestDistSq = DistSq;
					BestSlot = Slot;
				}
			}
			HoveredBuildSlot = BestSlot;
		}
	}
}

// creates / binds the screen damage vignette
void ACaveboundPlayerController::EnsureDamageVignette()
{
	if (!DamageVignette)
	{
		DamageVignette = NewObject<UCaveboundDamageVignette>(this);
	}
	if (DamageVignette)
	{
		DamageVignette->Ensure(GetWorld());
	}
}

// flashes red edges when the player takes damage
void ACaveboundPlayerController::PulseDamageVignette()
{
	EnsureDamageVignette();
	if (DamageVignette)
	{
		DamageVignette->PulseDamage(0.8f, 0.6f);
	}
}

// flashes green edges when wood is collected
void ACaveboundPlayerController::PulseWoodVignette()
{
	EnsureDamageVignette();
	if (DamageVignette)
	{
		DamageVignette->PulseWood(0.55f, 0.35f);
	}
}

// flashes grey edges when rock is collected
void ACaveboundPlayerController::PulseRockVignette()
{
	EnsureDamageVignette();
	if (DamageVignette)
	{
		DamageVignette->PulseRock(0.55f, 0.35f);
	}
}

// darkens the screen as the sacred tree loses health
void ACaveboundPlayerController::SyncTreeScreenDarken()
{
	EnsureDamageVignette();
	if (!DamageVignette)
	{
		return;
	}

	float HealthFrac = 1.f;
	if (UWorld* World = GetWorld())
	{
		if (ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>())
		{
			HealthFrac = GameMode->GetTreeHealthPercent();
		}
	}
	DamageVignette->SetTreeHealthFraction(HealthFrac);
}

// shows / hides the hover health card at the mouse
void ACaveboundPlayerController::UpdateHoverInfoWidget()
{
	if (!HoverInfo)
	{
		return;
	}

	if (!HasHoveredHealthTarget())
	{
		HoverInfo->HideInfo();
		SetHoverInfoAsCursor(false);
		return;
	}

	// viewport slate space — GetMousePosition() is pixels and offsets the card
	const FVector2D Mouse = UWidgetLayoutLibrary::GetMousePositionOnViewport(this);
	HoverInfo->ShowAt(
		Mouse,
		GetHoveredDisplayName(),
		GetHoveredHealth(),
		GetHoveredMaxHealth());
	SetHoverInfoAsCursor(true);
}

// swaps to invisible software cursor while hover card is up
void ACaveboundPlayerController::SetHoverInfoAsCursor(bool bActive)
{
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->SetUseSoftwareCursorWidgets(true);
	}

	auto SetPlatformCursorVisible = [](bool bVisible)
	{
		if (!FSlateApplication::IsInitialized())
		{
			return;
		}
		if (TSharedPtr<ICursor> PlatformCursor = FSlateApplication::Get().GetPlatformCursor())
		{
			PlatformCursor->Show(bVisible);
		}
	};

	if (bActive)
	{
		if (!EmptyCursor)
		{
			EmptyCursor = CreateWidget<UCaveboundEmptyCursor>(this, UCaveboundEmptyCursor::StaticClass());
		}
		if (EmptyCursor)
		{
			SetMouseCursorWidget(EMouseCursor::Custom, EmptyCursor);
			if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
			{
				Viewport->SetSoftwareCursorWidget(EMouseCursor::Custom, EmptyCursor);
			}
		}
		CurrentMouseCursor = EMouseCursor::Custom;
		bShowMouseCursor = false;
		SetPlatformCursorVisible(false);
		bHoverInfoAsCursor = true;
	}
	else if (bHoverInfoAsCursor)
	{
		bHoverInfoAsCursor = false;
		CurrentMouseCursor = EMouseCursor::Default;
		bShowMouseCursor = true;
		SetPlatformCursorVisible(true);
	}
}

bool ACaveboundPlayerController::HasHoveredHealthTarget() const
{
	const AActor* Actor = HoveredHealthActor.Get();
	return Actor && Actor->Implements<UCaveboundHoverHealth>();
}

float ACaveboundPlayerController::GetHoveredHealth() const
{
	AActor* Actor = HoveredHealthActor.Get();
	if (!Actor || !Actor->Implements<UCaveboundHoverHealth>())
	{
		return 0.f;
	}

	return ICaveboundHoverHealth::Execute_GetHoverHealth(Actor);
}

float ACaveboundPlayerController::GetHoveredMaxHealth() const
{
	AActor* Actor = HoveredHealthActor.Get();
	if (!Actor || !Actor->Implements<UCaveboundHoverHealth>())
	{
		return 0.f;
	}

	return ICaveboundHoverHealth::Execute_GetHoverMaxHealth(Actor);
}

FText ACaveboundPlayerController::GetHoveredDisplayName() const
{
	AActor* Actor = HoveredHealthActor.Get();
	if (!Actor || !Actor->Implements<UCaveboundHoverHealth>())
	{
		return FText::GetEmpty();
	}

	return ICaveboundHoverHealth::Execute_GetHoverDisplayName(Actor);
}

FText ACaveboundPlayerController::GetHoveredHealthText() const
{
	const float Health = GetHoveredHealth();
	const float MaxHealth = GetHoveredMaxHealth();
	return FText::FromString(FString::Printf(
		TEXT("%d / %d"),
		FMath::RoundToInt(Health),
		FMath::RoundToInt(MaxHealth)));
}

void ACaveboundPlayerController::StripSlotPlaceKeys()
{
	auto ShouldRemove = [this](const FInputKeyBinding& Binding)
	{
		const bool bPlaceKey = Binding.Chord.Key == EKeys::F || Binding.Chord.Key == EKeys::B;
		const bool bPicker = (Binding.Chord.Key == EKeys::B || Binding.Chord.Key == EKeys::F) && Binding.KeyDelegate.IsBoundToObject(this);
		return bPlaceKey && !bPicker;
	};

	if (InputComponent)
	{
		InputComponent->KeyBindings.RemoveAll(ShouldRemove);
	}

	UWorld* World = GetWorld();
	UClass* SlotClass = LoadClass<AActor>(nullptr, TEXT("/Game/Blueprints/BP_BuildSlot.BP_BuildSlot_C"));
	FObjectProperty* InputProperty = FindFProperty<FObjectProperty>(AActor::StaticClass(), TEXT("InputComponent"));
	if (!World || !SlotClass || !InputProperty)
	{
		return;
	}

	TArray<AActor*> Slots;
	UGameplayStatics::GetAllActorsOfClass(World, SlotClass, Slots);
	for (AActor* Slot : Slots)
	{
		UInputComponent* SlotInput = Cast<UInputComponent>(InputProperty->GetObjectPropertyValue_InContainer(Slot));
		if (SlotInput)
		{
			SlotInput->KeyBindings.RemoveAll(ShouldRemove);
		}
	}
}

// B key — opens / closes the place-defender menu
void ACaveboundPlayerController::HandleBuildKey()
{
	if (bPauseMenuOpen || bGameOverScreenOpen || IsTutorialActive())
	{
		return;
	}

	if (BuildMenu && BuildMenu->IsInViewport())
	{
		CloseBuildMenu();
		return;
	}

	if (DefenderMenu && DefenderMenu->IsInViewport())
	{
		CloseDefenderMenu();
	}

	if (IsInGracePeriod())
	{
		NotifyGracePhaseBlocked();
		return;
	}

	if (AActor* Slot = FindBuildSlotForMenu())
	{
		OpenBuildMenu(Slot);
	}
}

// F key — opens / closes the upgrade / repair / disarm menu
void ACaveboundPlayerController::HandleDefenderKey()
{
	if (bPauseMenuOpen || bGameOverScreenOpen || IsTutorialActive())
	{
		return;
	}

	if (DefenderMenu && DefenderMenu->IsInViewport())
	{
		CloseDefenderMenu();
		return;
	}

	if (BuildMenu && BuildMenu->IsInViewport())
	{
		CloseBuildMenu();
	}

	TArray<AActor*> OccupiedSlots;
	AActor* Closest = nullptr;
	CollectOccupiedSlots(OccupiedSlots, Closest);
	if (Closest)
	{
		OpenDefenderMenu(Closest);
	}
}

// shows the defender picker anchored above a build slot
void ACaveboundPlayerController::OpenBuildMenu(AActor* Slot)
{
	if (!Slot || bGameOverScreenOpen)
	{
		return;
	}

	if (IsInGracePeriod())
	{
		NotifyGracePhaseBlocked();
		return;
	}

	if (!BuildMenu)
	{
		BuildMenu = CreateWidget<UCaveboundBuildMenu>(this, UCaveboundBuildMenu::StaticClass());
	}

	if (!BuildMenu)
	{
		return;
	}

	MenuSlot = Slot;
	BuildMenu->OpenForSlot(Slot);
	if (BuildPrompt)
	{
		BuildPrompt->SetHoverVisible(false);
	}
	if (!BuildMenu->IsInViewport())
	{
		BuildMenu->AddToViewport(15);
	}

	FVector2D ScreenPosition;
	if (GetSlotScreenPosition(Slot, 200.f, ScreenPosition))
	{
		BuildMenu->SetHoverScreenPosition(ScreenPosition);
		BuildMenu->SetHoverVisible(true);
	}

	ApplyMenuInputMode();
}

void ACaveboundPlayerController::CloseBuildMenu()
{
	MenuSlot = nullptr;
	if (BuildMenu && BuildMenu->IsInViewport())
	{
		BuildMenu->RemoveFromParent();
	}

	ApplyGameplayInputMode();
}

// shows upgrade / repair / disarm for the defender on a slot
void ACaveboundPlayerController::OpenDefenderMenu(AActor* Slot)
{
	ACaveboundTurret* Defender = GetDefenderOnSlot(Slot);
	if (!Slot || !Defender || Defender->IsDestroyed())
	{
		return;
	}

	if (BuildMenu && BuildMenu->IsInViewport())
	{
		CloseBuildMenu();
	}

	if (!DefenderMenu)
	{
		DefenderMenu = CreateWidget<UCaveboundDefenderMenu>(this, UCaveboundDefenderMenu::StaticClass());
	}

	if (!DefenderMenu)
	{
		return;
	}

	MenuSlot = Slot;
	DefenderMenu->OpenForDefender(Slot, Defender);
	if (BuildPrompt)
	{
		BuildPrompt->SetHoverVisible(false);
	}
	if (!DefenderMenu->IsInViewport())
	{
		DefenderMenu->AddToViewport(15);
	}

	FVector2D ScreenPosition;
	if (GetSlotScreenPosition(Slot, 200.f, ScreenPosition))
	{
		DefenderMenu->SetHoverScreenPosition(ScreenPosition);
		DefenderMenu->SetHoverVisible(true);
	}

	ApplyMenuInputMode();
}

void ACaveboundPlayerController::CloseDefenderMenu()
{
	MenuSlot = nullptr;
	if (DefenderMenu && DefenderMenu->IsInViewport())
	{
		DefenderMenu->RemoveFromParent();
	}

	ApplyGameplayInputMode();
}

ACaveboundTurret* ACaveboundPlayerController::GetDefenderOnSlot(AActor* Slot) const
{
	if (!Slot)
	{
		return nullptr;
	}

	const FObjectProperty* DefenderActor = FindFProperty<FObjectProperty>(Slot->GetClass(), TEXT("OccupyingDefender"));
	if (!DefenderActor)
	{
		return nullptr;
	}

	return Cast<ACaveboundTurret>(DefenderActor->GetObjectPropertyValue_InContainer(Slot));
}

void ACaveboundPlayerController::CollectOccupiedSlots(TArray<AActor*>& OutSlots, AActor*& OutClosest) const
{
	OutSlots.Reset();
	OutClosest = nullptr;

	UWorld* World = GetWorld();
	UClass* SlotClass = LoadClass<AActor>(nullptr, TEXT("/Game/Blueprints/BP_BuildSlot.BP_BuildSlot_C"));
	if (!World || !SlotClass)
	{
		return;
	}

	TArray<AActor*> Slots;
	UGameplayStatics::GetAllActorsOfClass(World, SlotClass, Slots);

	const APawn* PlayerPawn = GetPawn();
	float BestDistanceSq = TNumericLimits<float>::Max();
	for (AActor* Slot : Slots)
	{
		if (!Slot)
		{
			continue;
		}

		const FBoolProperty* Nearby = FindFProperty<FBoolProperty>(Slot->GetClass(), TEXT("PlayerNearby"));
		const FBoolProperty* Occupied = FindFProperty<FBoolProperty>(Slot->GetClass(), TEXT("IsOccupied"));
		if (!Nearby || !Nearby->GetPropertyValue_InContainer(Slot))
		{
			continue;
		}
		if (!Occupied || !Occupied->GetPropertyValue_InContainer(Slot))
		{
			continue;
		}

		ACaveboundTurret* Defender = GetDefenderOnSlot(Slot);
		if (!Defender || Defender->IsDestroyed())
		{
			continue;
		}

		OutSlots.Add(Slot);

		const float DistanceSq = PlayerPawn
			? FVector::DistSquared(PlayerPawn->GetActorLocation(), Slot->GetActorLocation())
			: 0.f;
		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			OutClosest = Slot;
		}
	}
}

// runs upgrade (0), repair (1), or disarm (2) from the defender menu
bool ACaveboundPlayerController::UseDefenderAction(AActor* Slot, int32 Action, FString& OutFailure)
{
	OutFailure.Reset();
	ACaveboundTurret* Defender = GetDefenderOnSlot(Slot);
	UWorld* World = GetWorld();
	ACaveboundGameMode* GameMode = World ? World->GetAuthGameMode<ACaveboundGameMode>() : nullptr;
	if (!Defender || !GameMode || Defender->IsDestroyed())
	{
		OutFailure = TEXT("Defender is gone.");
		return false;
	}

	// upgrade — spends resources, levels up, plays beam vfx
	if (Action == 0)
	{
		if (Defender->GetUpgradeLevel() >= Defender->GetMaxUpgradeLevel())
		{
			OutFailure = TEXT("Already fully upgraded.");
			return false;
		}
		if (GameMode->GetRock() < Defender->GetNextUpgradeRockCost() || GameMode->GetWood() < Defender->GetNextUpgradeWoodCost())
		{
			OutFailure = TEXT("Not enough resources.");
			return false;
		}

		const int32 NextLevel = Defender->GetUpgradeLevel() + 1;
		if (!Defender->CommitUpgradePurchase(GameMode))
		{
			OutFailure = TEXT("Not enough resources.");
			return false;
		}

		// beam tint + pop, no scale fade-in (build-only)
		Defender->ApplyCommittedUpgrade();
		ACaveboundPlaceBeamVFX::SpawnAt(
			World,
			Slot->GetActorLocation(),
			ACaveboundPlaceBeamVFX::ColorForUpgrade(NextLevel),
			Defender,
			false);
		return true;
	}

	// repair — spends wood to restore full health
	if (Action == 1)
	{
		if (Defender->GetRepairCost() <= 0)
		{
			OutFailure = TEXT("Already at full health.");
			return false;
		}
		if (!Defender->TryRepair(GameMode))
		{
			OutFailure = TEXT("Not enough wood.");
			return false;
		}
		return true;
	}

	// disarm — refunds resources and removes the defender
	int32 refundWood = 0;
	int32 refundRock = 0;
	Defender->GetDisarmRefund(refundWood, refundRock);
	GameMode->GiveWood(refundWood);
	GameMode->GiveRock(refundRock);
	Defender->Destroy();
	return true;
}

void ACaveboundPlayerController::CollectInteractableSlots(TArray<AActor*>& OutSlots, AActor*& OutClosest) const
{
	OutSlots.Reset();
	OutClosest = nullptr;

	UWorld* World = GetWorld();
	UClass* SlotClass = LoadClass<AActor>(nullptr, TEXT("/Game/Blueprints/BP_BuildSlot.BP_BuildSlot_C"));
	if (!World || !SlotClass)
	{
		return;
	}

	TArray<AActor*> Slots;
	UGameplayStatics::GetAllActorsOfClass(World, SlotClass, Slots);

	const APawn* PlayerPawn = GetPawn();
	float BestDistanceSq = TNumericLimits<float>::Max();
	for (AActor* Slot : Slots)
	{
		if (!Slot)
		{
			continue;
		}

		const FBoolProperty* Nearby = FindFProperty<FBoolProperty>(Slot->GetClass(), TEXT("PlayerNearby"));
		const FBoolProperty* Occupied = FindFProperty<FBoolProperty>(Slot->GetClass(), TEXT("IsOccupied"));
		if (!Nearby || !Nearby->GetPropertyValue_InContainer(Slot))
		{
			continue;
		}
		if (Occupied && Occupied->GetPropertyValue_InContainer(Slot))
		{
			continue;
		}

		OutSlots.Add(Slot);

		const float DistanceSq = PlayerPawn
			? FVector::DistSquared(PlayerPawn->GetActorLocation(), Slot->GetActorLocation())
			: 0.f;
		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			OutClosest = Slot;
		}
	}
}

AActor* ACaveboundPlayerController::FindBuildSlotForMenu() const
{
	TArray<AActor*> NearbySlots;
	AActor* Closest = nullptr;
	CollectInteractableSlots(NearbySlots, Closest);
	return Closest;
}

void ACaveboundPlayerController::SetSlotHighlighted(AActor* Slot, bool bHighlighted)
{
	if (!Slot)
	{
		return;
	}

	if (bHighlighted && !SlotHighlightMaterial)
	{
		SlotHighlightMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/Game/Assets/Materials/M_Highlight.M_Highlight"));
	}

	TArray<UStaticMeshComponent*> Meshes;
	Slot->GetComponents<UStaticMeshComponent>(Meshes);
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		if (!Mesh)
		{
			continue;
		}
		if (bHighlighted)
		{
			Mesh->SetOverlayMaterial(SlotHighlightMaterial.Get());
		}
		else
		{
			CaveboundModelOutline::RestoreMesh(Mesh);
		}
	}
}

bool ACaveboundPlayerController::GetSlotScreenPosition(const AActor* Slot, float Height, FVector2D& OutScreenPosition) const
{
	if (!Slot)
	{
		return false;
	}

	FVector2D PixelPosition;
	const FVector WorldPosition = Slot->GetActorLocation() + FVector(0.f, 0.f, Height);
	if (!ProjectWorldLocationToScreen(WorldPosition, PixelPosition, true))
	{
		return false;
	}

	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
	OutScreenPosition = ViewportScale > 0.f ? PixelPosition / ViewportScale : PixelPosition;
	return true;
}

void ACaveboundPlayerController::UpdateBuildSlotPresentation()
{
	if (bPauseMenuOpen)
	{
		return;
	}

	TArray<AActor*> NearbySlots;
	AActor* Closest = nullptr;
	CollectInteractableSlots(NearbySlots, Closest);

	for (int32 Index = HighlightedSlots.Num() - 1; Index >= 0; --Index)
	{
		AActor* Highlighted = HighlightedSlots[Index].Get();
		if (!Highlighted || !NearbySlots.Contains(Highlighted))
		{
			SetSlotHighlighted(Highlighted, false);
			HighlightedSlots.RemoveAt(Index);
		}
	}

	for (AActor* Slot : NearbySlots)
	{
		const bool bAlreadyHighlighted = HighlightedSlots.ContainsByPredicate(
			[Slot](const TWeakObjectPtr<AActor>& Existing)
			{
				return Existing.Get() == Slot;
			});
		if (!bAlreadyHighlighted)
		{
			SetSlotHighlighted(Slot, true);
			HighlightedSlots.Add(Slot);
		}
	}

	// while a menu is open, keep it pinned above the slot and hide the key prompt
	const bool bBuildMenuOpen = BuildMenu && BuildMenu->IsInViewport();
	const bool bDefenderMenuOpen = DefenderMenu && DefenderMenu->IsInViewport();
	if (bBuildMenuOpen || bDefenderMenuOpen)
	{
		if (BuildPrompt)
		{
			BuildPrompt->SetHoverVisible(false);
		}

		FVector2D ScreenPosition;
		if (bBuildMenuOpen && GetSlotScreenPosition(MenuSlot.Get(), 210.f, ScreenPosition))
		{
			BuildMenu->SetHoverScreenPosition(ScreenPosition);
			BuildMenu->SetHoverVisible(true);
		}
		else if (bDefenderMenuOpen && GetSlotScreenPosition(MenuSlot.Get(), 210.f, ScreenPosition))
		{
			DefenderMenu->SetHoverScreenPosition(ScreenPosition);
			DefenderMenu->SetHoverVisible(true);
		}
		return;
	}

	// picks B (empty) or F (occupied) prompt for the nearest slot
	TArray<AActor*> OccupiedSlots;
	AActor* OccupiedClosest = nullptr;
	CollectOccupiedSlots(OccupiedSlots, OccupiedClosest);

	AActor* PromptSlot = Closest;
	FString PromptKey = TEXT("B");
	const APawn* PlayerPawn = GetPawn();
	if (OccupiedClosest && PlayerPawn)
	{
		const float occupiedDistance = FVector::DistSquared(PlayerPawn->GetActorLocation(), OccupiedClosest->GetActorLocation());
		const float emptyDistance = Closest
			? FVector::DistSquared(PlayerPawn->GetActorLocation(), Closest->GetActorLocation())
			: TNumericLimits<float>::Max();
		if (occupiedDistance <= emptyDistance)
		{
			PromptSlot = OccupiedClosest;
			PromptKey = TEXT("F");
		}
	}
	else if (OccupiedClosest)
	{
		PromptSlot = OccupiedClosest;
		PromptKey = TEXT("F");
	}

	if (!PromptSlot)
	{
		if (BuildPrompt)
		{
			BuildPrompt->SetHoverVisible(false);
		}
		return;
	}

	if (!BuildPrompt)
	{
		BuildPrompt = CreateWidget<UCaveboundBuildPrompt>(this, UCaveboundBuildPrompt::StaticClass());
		if (BuildPrompt)
		{
			BuildPrompt->AddToViewport(12);
		}
	}

	if (!BuildPrompt)
	{
		return;
	}

	BuildPrompt->SetPromptKey(PromptKey);

	FVector2D ScreenPosition;
	if (GetSlotScreenPosition(PromptSlot, 150.f, ScreenPosition))
	{
		BuildPrompt->SetHoverScreenPosition(ScreenPosition);
		BuildPrompt->SetHoverVisible(true);
	}
	else
	{
		BuildPrompt->SetHoverVisible(false);
	}
}

// spends wood, spawns the defender, and plays place-beam vfx
bool ACaveboundPlayerController::PlaceDefenderAtSlot(AActor* Slot, TSubclassOf<AActor> DefenderClass)
{
	UWorld* World = GetWorld();
	if (!Slot || !DefenderClass || !World || bGameOverScreenOpen)
	{
		return false;
	}

	ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>();
	if (!GameMode || GameMode->IsRoundIdle())
	{
		NotifyGracePhaseBlocked();
		return false;
	}

	const FBoolProperty* Occupied = FindFProperty<FBoolProperty>(Slot->GetClass(), TEXT("IsOccupied"));
	if (Occupied && Occupied->GetPropertyValue_InContainer(Slot))
	{
		return false;
	}

	const int32 Cost = ACaveboundTurret::GetCostForClass(DefenderClass);
	if (Cost <= 0 || !GameMode->UseWood(Cost))
	{
		return false;
	}

	if (FBoolProperty* OccupiedWrite = FindFProperty<FBoolProperty>(Slot->GetClass(), TEXT("IsOccupied")))
	{
		OccupiedWrite->SetPropertyValue_InContainer(Slot, true);
	}

	const FTransform SlotTransform = Slot->GetActorTransform();
	AActor* Spawned = SpawnPlacedDefender(Slot, DefenderClass, SlotTransform);
	if (!Spawned)
	{
		GameMode->GiveWood(Cost);
		ClearSlotOccupied(Slot);
		return false;
	}

	// fades the defender in with the yellow beam, then pops it in
	ACaveboundPlaceBeamVFX::SpawnAt(
		World,
		Slot->GetActorLocation(),
		ACaveboundPlaceBeamVFX::ColorForBuild(),
		Spawned);
	return true;
}

// spawns a defender actor and wires it to the build slot
AActor* ACaveboundPlayerController::SpawnPlacedDefender(
	AActor* Slot,
	TSubclassOf<AActor> DefenderClass,
	const FTransform& SlotTransform)
{
	UWorld* World = GetWorld();
	if (!Slot || !DefenderClass || !World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AActor* Spawned = World->SpawnActor<AActor>(DefenderClass, SlotTransform, SpawnParams);
	if (!Spawned)
	{
		return nullptr;
	}

	if (FObjectProperty* TerrainProp = FindFProperty<FObjectProperty>(Spawned->GetClass(), TEXT("Terrain")))
	{
		if (!TerrainProp->GetObjectPropertyValue_InContainer(Spawned))
		{
			UClass* TerrainClass = LoadClass<AActor>(
				nullptr,
				TEXT("/Game/Blueprints/BP_ProceduralTerrain.BP_ProceduralTerrain_C"));
			if (TerrainClass)
			{
				TArray<AActor*> Terrains;
				UGameplayStatics::GetAllActorsOfClass(World, TerrainClass, Terrains);
				if (Terrains.Num() > 0 && Terrains[0])
				{
					TerrainProp->SetObjectPropertyValue_InContainer(Spawned, Terrains[0]);
				}
			}
		}
	}

	if (FBoolProperty* OccupiedWrite = FindFProperty<FBoolProperty>(Slot->GetClass(), TEXT("IsOccupied")))
	{
		OccupiedWrite->SetPropertyValue_InContainer(Slot, true);
	}
	if (FObjectProperty* DefenderActor = FindFProperty<FObjectProperty>(Slot->GetClass(), TEXT("OccupyingDefender")))
	{
		DefenderActor->SetObjectPropertyValue_InContainer(Slot, Spawned);
	}

	Spawned->OnDestroyed.AddDynamic(this, &ACaveboundPlayerController::HandlePlacedDefenderDestroyed);
	DefendersToSlots.Add(Spawned, Slot);
	return Spawned;
}

void ACaveboundPlayerController::ClearSlotOccupied(AActor* Slot)
{
	if (!Slot)
	{
		return;
	}

	if (FBoolProperty* Occupied = FindFProperty<FBoolProperty>(Slot->GetClass(), TEXT("IsOccupied")))
	{
		Occupied->SetPropertyValue_InContainer(Slot, false);
	}
	if (FObjectProperty* DefenderActor = FindFProperty<FObjectProperty>(Slot->GetClass(), TEXT("OccupyingDefender")))
	{
		DefenderActor->SetObjectPropertyValue_InContainer(Slot, nullptr);
	}
}

void ACaveboundPlayerController::HandlePlacedDefenderDestroyed(AActor* DestroyedActor)
{
	if (TObjectPtr<AActor>* Slot = DefendersToSlots.Find(DestroyedActor))
	{
		ClearSlotOccupied(*Slot);
	}
	DefendersToSlots.Remove(DestroyedActor);
}
