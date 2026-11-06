#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CaveboundScreenFade.generated.h"

class UBorder;

/** Full-screen black fade used for match start. */
UCLASS()
class CAVEBOUND_API UCaveboundScreenFade : public UUserWidget
{
	GENERATED_BODY()

public:
	void StartFadeIn(float Duration = 1.5f);
	void TickFade(float DeltaTime);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY()
	TObjectPtr<UBorder> FadeBorder;

	float FadeDuration = 1.5f;
	float FadeElapsed = 0.f;
	bool bFading = false;
};
