#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaveboundHarvestBar.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * Bar shown over a rock deposit while the player is harvesting it.
 */
UCLASS()
class CAVEBOUND_API UCaveboundHarvestBar : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetHarvest(int32 RockAmount, float Progress);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY()
	TObjectPtr<UTextBlock> AmountText;

	UPROPERTY()
	TObjectPtr<UProgressBar> Bar;
};
