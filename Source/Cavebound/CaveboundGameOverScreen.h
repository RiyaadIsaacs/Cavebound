#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaveboundGameOverScreen.generated.h"

class UButton;

/** Game over overlay when the tree dies — Restart / Quit. */
UCLASS()
class CAVEBOUND_API UCaveboundGameOverScreen : public UUserWidget
{
	GENERATED_BODY()

public:
	void HandleRestart();
	void HandleQuit();

protected:
	virtual void NativeOnInitialized() override;

	void AddActionButton(class UVerticalBox* Body, const FString& Label, int32 Action);

	UPROPERTY()
	TArray<TObjectPtr<UObject>> ButtonProxies;
};

UCLASS()
class UCaveboundGameOverActionProxy : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UCaveboundGameOverScreen> Menu;

	int32 Action = 0;

	UFUNCTION()
	void HandleClicked();
};
