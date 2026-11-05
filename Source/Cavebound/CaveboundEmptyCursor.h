#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaveboundEmptyCursor.generated.h"

/** 1px invisible software cursor used to hide the OS arrow while the hover card is shown. */
UCLASS()
class CAVEBOUND_API UCaveboundEmptyCursor : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
};
