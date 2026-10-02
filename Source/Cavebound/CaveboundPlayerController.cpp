#include "CaveboundPlayerController.h"
#include "CaveboundBuildMenu.h"
#include "CaveboundBuildPrompt.h"
#include "CaveboundCharacter.h"
#include "CaveboundGameMode.h"
#include "CaveboundHoverHealth.h"
#include "CaveboundTree.h"
#include "CaveboundTurret.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Materials/MaterialInterface.h"
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

	if (ACaveboundGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ACaveboundGameMode>() : nullptr)
	{
		GameMode->EnsureGameplayReady();
	}

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

	// The slot blueprint still has an instant-place key. Drop it so B only opens the picker.
	StripSlotPlaceKeys();
	UpdateBuildSlotPresentation();

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
}

void ACaveboundPlayerController::CloseBuildMenu()
{
	MenuSlot = nullptr;
	if (BuildMenu && BuildMenu->IsInViewport())
	{
		BuildMenu->RemoveFromParent();
	}
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
