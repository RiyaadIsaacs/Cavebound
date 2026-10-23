#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaveboundDefenderMenu.generated.h"

class UBorder;
class UButton;
class UCanvasPanelSlot;
class UTextBlock;
class UVerticalBox;
class ACaveboundTurret;

UCLASS()
class UCaveboundDefenderActionProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<class UCaveboundDefenderMenu> Menu;

	// 0 upgrade, 1 repair, 2 disarm
	int32 Action = 0;

	UFUNCTION()
	void HandleClicked();
};

/**
 * F menu on a built defender. Upgrade, repair, or disarm.
 */
UCLASS()
class CAVEBOUND_API UCaveboundDefenderMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetHoverScreenPosition(FVector2D Position);
	void SetHoverVisible(bool bVisible);
	void OpenForDefender(AActor* BuildSlot, ACaveboundTurret* Defender);
	bool IsPointerInsideMenu() const;

	void HandleAction(int32 Action);

protected:
	virtual void NativeOnInitialized() override;

	void AddAction(const FString& Label, const FString& Detail, int32 Action);

	UPROPERTY()
	TObjectPtr<UBorder> FrameWidget;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY()
	TObjectPtr<UTextBlock> ResourceText;

	UPROPERTY()
	TObjectPtr<UVerticalBox> ChoiceBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> PanelSlot;

	UPROPERTY()
	TArray<TObjectPtr<UCaveboundDefenderActionProxy>> ActionProxies;

	TWeakObjectPtr<AActor> TargetSlot;
	TWeakObjectPtr<ACaveboundTurret> TargetDefender;
};
