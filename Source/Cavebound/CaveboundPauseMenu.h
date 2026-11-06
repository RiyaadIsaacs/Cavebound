#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaveboundPauseMenu.generated.h"

class UButton;

/** Styled pause overlay — Resume / Restart / Quit. Replaces WBP_PauseMenu. */
UCLASS()
class CAVEBOUND_API UCaveboundPauseMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	void HandleResume();
	void HandleRestart();
	void HandleQuit();

protected:
	virtual void NativeOnInitialized() override;

	void AddActionButton(class UVerticalBox* Body, const FString& Label, int32 Action);

	UPROPERTY()
	TArray<TObjectPtr<UObject>> ButtonProxies;
};

UCLASS()
class UCaveboundPauseActionProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UCaveboundPauseMenu> Menu;

	int32 Action = 0;

	UFUNCTION()
	void HandleClicked();
};
