#include "CaveboundPlayerController.h"
#include "CaveboundCharacter.h"
#include "CaveboundTree.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"

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
	}
}

void ACaveboundPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Backup if Enhanced Input assets have no mapping: raw left-click still moves.
	if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		OnClickMove();
	}
}

void ACaveboundPlayerController::OnClickMove()
{
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
