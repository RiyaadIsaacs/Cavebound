#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CaveboundHoverHealth.generated.h"

UINTERFACE(BlueprintType)
class CAVEBOUND_API UCaveboundHoverHealth : public UInterface
{
	GENERATED_BODY()
};

/**
 * Actors the HUD can show HP for on mouse hover 
 */
class CAVEBOUND_API ICaveboundHoverHealth
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cavebound")
	float GetHoverHealth() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cavebound")
	float GetHoverMaxHealth() const;
};
