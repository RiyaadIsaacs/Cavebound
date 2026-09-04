#include "CaveboundPlayerController.h"
#include "CaveboundCharacter.h"
#include "CaveboundHoverHealth.h"
#include "CaveboundTree.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

ACaveboundPlayerController::ACaveboundPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	DefaultMouseCursor = EMouseCursor::Default;
}

void ACaveboundPlayerController::EnsureClickMoveInput()
{
	if (!ClickMoveAction)
	{
		ClickMoveAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_ClickMove.IA_ClickMove"));
	}

	if (!DefaultMappingContext)
	{
		DefaultMappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Player.IMC_Player"));
	}

	if (!ClickMoveAction)
	{
		ClickMoveAction = NewObject<UInputAction>(this, TEXT("IA_ClickMove"));
		ClickMoveAction->ValueType = EInputActionValueType::Boolean;
	}

	if (!DefaultMappingContext)
	{
		DefaultMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Player"));
	}

	// Content IMC can exist but have no keys (asset creation missed the LMB bind).
	// Always make sure left mouse fires ClickMoveAction.
	if (DefaultMappingContext && ClickMoveAction && DefaultMappingContext->GetMappings().Num() == 0)
	{
		DefaultMappingContext->MapKey(ClickMoveAction, EKeys::LeftMouseButton);
	}
}

void ACaveboundPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ApplyGameplayInputMode();

	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
		Viewport->SetHideCursorDuringCapture(false);
	}

	EnsureClickMoveInput();
	ShowHUD();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ACaveboundPlayerController::ShowHUD()
{
	if (!HUDWidgetClass || HUDWidget)
	{
		return;
	}

	HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
	if (HUDWidget)
	{
		HUDWidget->AddToViewport(0);
	}
}

void ACaveboundPlayerController::ApplyGameplayInputMode()
{
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ACaveboundPlayerController::ApplyPauseInputMode()
{
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (PauseMenuWidget)
	{
		InputMode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	}
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ACaveboundPlayerController::ShowPauseMenu()
{
	if (!PauseMenuWidgetClass)
	{
		return;
	}

	if (!PauseMenuWidget)
	{
		PauseMenuWidget = CreateWidget<UUserWidget>(this, PauseMenuWidgetClass);
	}

	if (PauseMenuWidget && !PauseMenuWidget->IsInViewport())
	{
		// Above the gameplay HUD
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
	if (bPauseMenuOpen)
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
	ApplyGameplayInputMode();
}

void ACaveboundPlayerController::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void ACaveboundPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	EnsureClickMoveInput();

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (ClickMoveAction)
		{
			EnhancedInput->BindAction(
				ClickMoveAction,
				ETriggerEvent::Started,
				this,
				&ACaveboundPlayerController::OnClickMove);
		}
	}

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ACaveboundPlayerController::OnClickMove);

		// bExecuteWhenPaused so Escape/Tab can resume while the world is paused
		FInputKeyBinding& EscapeBinding = InputComponent->BindKey(
			EKeys::Escape,
			IE_Pressed,
			this,
			&ACaveboundPlayerController::TogglePauseMenu);
		EscapeBinding.bExecuteWhenPaused = true;

		FInputKeyBinding& TabBinding = InputComponent->BindKey(
			EKeys::Tab,
			IE_Pressed,
			this,
			&ACaveboundPlayerController::TogglePauseMenu);
		TabBinding.bExecuteWhenPaused = true;
	}
}

void ACaveboundPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (bPauseMenuOpen)
	{
		return;
	}

	// Backup if Enhanced Input assets have no mapping: raw left-click still moves.
	if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		OnClickMove();
	}

	if (IsInputKeyDown(EKeys::RightMouseButton))
	{
		float MouseX = 0.f;
		float MouseY = 0.f;
		if (GetMousePosition(MouseX, MouseY))
		{
			if (bOrbitingCamera)
			{
				const float DeltaX = MouseX - LastOrbitMouseX;
				const float DeltaY = MouseY - LastOrbitMouseY;
				if (ACaveboundCharacter* PlayerCharacter = Cast<ACaveboundCharacter>(GetPawn()))
				{
					PlayerCharacter->OrbitCamera(DeltaX, DeltaY);
				}
			}
			LastOrbitMouseX = MouseX;
			LastOrbitMouseY = MouseY;
			bOrbitingCamera = true;
		}
	}
	else
	{
		bOrbitingCamera = false;
	}

	UpdateHoveredHealthTarget();
}

void ACaveboundPlayerController::OnClickMove()
{
	if (bPauseMenuOpen)
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
		else
		{
			PlayerCharacter->SetMoveDestination(Hit.ImpactPoint);
		}
	}
}

void ACaveboundPlayerController::UpdateHoveredHealthTarget()
{
	if (bOrbitingCamera)
	{
		return;
	}

	HoveredHealthActor = nullptr;

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

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HoverHealth), false, GetPawn());
	const FVector TraceEnd = WorldLocation + WorldDirection * 100000.f;
	if (!World->LineTraceSingleByChannel(Hit, WorldLocation, TraceEnd, ECC_Visibility, Params))
	{
		return;
	}

	AActor* HitActor = Hit.GetActor();
	if (!IsValid(HitActor) || HitActor == GetPawn())
	{
		return;
	}

	if (HitActor->Implements<UCaveboundHoverHealth>())
	{
		HoveredHealthActor = HitActor;
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
