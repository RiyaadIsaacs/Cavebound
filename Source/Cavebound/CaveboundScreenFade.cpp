#include "CaveboundScreenFade.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

void UCaveboundScreenFade::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetIsFocusable(false);

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FadeCanvas"));
	Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = Canvas;

	FadeBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FadeBorder"));
	FadeBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 1.f));
	FadeBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* FadeSlot = Canvas->AddChildToCanvas(FadeBorder))
	{
		FadeSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		FadeSlot->SetOffsets(FMargin(0.f));
	}
}

void UCaveboundScreenFade::StartFadeIn(float Duration)
{
	FadeDuration = FMath::Max(0.05f, Duration);
	FadeElapsed = 0.f;
	bFading = true;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (FadeBorder)
	{
		FadeBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 1.f));
	}
}

void UCaveboundScreenFade::TickFade(float DeltaTime)
{
	if (!bFading || !FadeBorder)
	{
		return;
	}

	FadeElapsed += DeltaTime;
	const float T = FMath::Clamp(FadeElapsed / FadeDuration, 0.f, 1.f);
	FadeBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 1.f - T));

	if (T >= 1.f)
	{
		bFading = false;
		SetVisibility(ESlateVisibility::Collapsed);
	}
}
