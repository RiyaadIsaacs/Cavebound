#include "CaveboundPlayerController.h"
#include "CaveboundBuildMenu.h"
#include "CaveboundBuildPrompt.h"
#include "CaveboundCharacter.h"
#include "CaveboundGameMode.h"
#include "CaveboundHoverHealth.h"
#include "CaveboundTree.h"
#include "CaveboundTurret.h"
#include "Components/InputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/StaticMeshComponent.h"
#include "Components/Widget.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/Slider.h"
#include "Components/EditableText.h"
#include "Components/EditableTextBox.h"
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
	if (!HUDWidgetClass || HUDWidget)
	{
		return;
	}

	HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
	if (HUDWidget)
	{
		HUDWidget->AddToViewport(0);
		ConfigureHudClickThrough();
	}
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

		if (Widget->IsA<UButton>()
			|| Widget->IsA<UCheckBox>()
			|| Widget->IsA<USlider>()
			|| Widget->IsA<UEditableText>()
			|| Widget->IsA<UEditableTextBox>())
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

	if (InputComponent)
	{
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

		InputComponent->BindKey(EKeys::B, IE_Pressed, this, &ACaveboundPlayerController::HandleBuildKey);
		InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ACaveboundPlayerController::RequestStartRound);
	}
}

void ACaveboundPlayerController::RequestStartRound()
{
	if (bPauseMenuOpen)
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

	if (bPauseMenuOpen)
	{
		bLeftMouseWasDown = IsMouseButtonHeld(EKeys::LeftMouseButton);
		bRightMouseWasDown = IsMouseButtonHeld(EKeys::RightMouseButton);
		return;
	}

	const bool bLMB = IsMouseButtonHeld(EKeys::LeftMouseButton);
	const bool bRMB = IsMouseButtonHeld(EKeys::RightMouseButton);

	float MouseX = 0.f;
	float MouseY = 0.f;
	const bool bMouseInViewport = GetMousePosition(MouseX, MouseY);

	if (bLMB && !bLeftMouseWasDown && bMouseInViewport)
	{
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
	if (bPauseMenuOpen)
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

	return false;
}

void ACaveboundPlayerController::OnClickMove()
{
	if (bPauseMenuOpen || IsPointerOverInteractiveUI())
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
	if (bOrbitingCamera || bPauseMenuOpen)
	{
		HoveredHealthActor = nullptr;
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
		const bool bPicker = Binding.Chord.Key == EKeys::B && Binding.KeyDelegate.IsBoundToObject(this);
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

void ACaveboundPlayerController::HandleBuildKey()
{
	if (bPauseMenuOpen)
	{
		return;
	}

	if (BuildMenu && BuildMenu->IsInViewport())
	{
		CloseBuildMenu();
		return;
	}

	if (AActor* Slot = FindBuildSlotForMenu())
	{
		OpenBuildMenu(Slot);
	}
}

void ACaveboundPlayerController::OpenBuildMenu(AActor* Slot)
{
	if (!Slot)
	{
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
		if (Mesh)
		{
			Mesh->SetOverlayMaterial(bHighlighted ? SlotHighlightMaterial.Get() : nullptr);
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

	if (BuildMenu && BuildMenu->IsInViewport() && MenuSlot.Get() != Closest)
	{
		CloseBuildMenu();
	}

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

	const bool bMenuOpen = BuildMenu && BuildMenu->IsInViewport();
	if (bMenuOpen)
	{
		if (BuildPrompt)
		{
			BuildPrompt->SetHoverVisible(false);
		}

		FVector2D ScreenPosition;
		if (GetSlotScreenPosition(MenuSlot.Get(), 210.f, ScreenPosition))
		{
			BuildMenu->SetHoverScreenPosition(ScreenPosition);
			BuildMenu->SetHoverVisible(true);
		}
		else
		{
			BuildMenu->SetHoverVisible(false);
		}
		return;
	}

	if (!Closest)
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

	FVector2D ScreenPosition;
	if (GetSlotScreenPosition(Closest, 150.f, ScreenPosition))
	{
		BuildPrompt->SetHoverScreenPosition(ScreenPosition);
		BuildPrompt->SetHoverVisible(true);
	}
	else
	{
		BuildPrompt->SetHoverVisible(false);
	}
}

bool ACaveboundPlayerController::PlaceDefenderAtSlot(AActor* Slot, TSubclassOf<AActor> DefenderClass)
{
	UWorld* World = GetWorld();
	if (!Slot || !DefenderClass || !World)
	{
		return false;
	}

	const FBoolProperty* Occupied = FindFProperty<FBoolProperty>(Slot->GetClass(), TEXT("IsOccupied"));
	if (Occupied && Occupied->GetPropertyValue_InContainer(Slot))
	{
		return false;
	}

	const int32 Cost = ACaveboundTurret::GetCostForClass(DefenderClass);
	ACaveboundGameMode* GameMode = World->GetAuthGameMode<ACaveboundGameMode>();
	if (Cost <= 0 || !GameMode || !GameMode->UseWood(Cost))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AActor* Spawned = World->SpawnActor<AActor>(DefenderClass, Slot->GetActorTransform(), SpawnParams);
	if (!Spawned)
	{
		GameMode->AddWood(Cost);
		return false;
	}

	// BP_DefenderBase.SetOccupied reads Terrain — assign the procedural terrain if the BP exposes it
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
	return true;
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
