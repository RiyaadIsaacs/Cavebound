#pragma once

#include "CoreMinimal.h"
#include "Styling/CoreStyle.h"
#include "Fonts/SlateFontInfo.h"

/**
 * Shared look for Cavebound C++ UI — dark warm panels, cream text, soft green accents.
 */
namespace CaveboundUIStyle
{
	inline FLinearColor PanelBg() { return FLinearColor(0.08f, 0.06f, 0.04f, 0.94f); }
	inline FLinearColor ButtonBg() { return FLinearColor(0.22f, 0.2f, 0.16f, 1.f); }
	inline FLinearColor ButtonBgActive() { return FLinearColor(0.35f, 0.42f, 0.22f, 1.f); }
	inline FLinearColor Title() { return FLinearColor(0.96f, 0.9f, 0.75f, 1.f); }
	inline FLinearColor Body() { return FLinearColor(0.82f, 0.74f, 0.55f, 1.f); }
	inline FLinearColor Accent() { return FLinearColor(0.85f, 1.f, 0.75f, 1.f); }
	inline FLinearColor Danger() { return FLinearColor(0.95f, 0.45f, 0.35f, 1.f); }
	inline FLinearColor HealthFill() { return FLinearColor(0.55f, 0.78f, 0.35f, 1.f); }
	inline FLinearColor HealthBack() { return FLinearColor(0.12f, 0.1f, 0.08f, 0.9f); }
	inline FLinearColor BarFill() { return FLinearColor(0.62f, 0.55f, 0.42f, 1.f); }

	inline FSlateFontInfo Font(int32 Size)
	{
		FSlateFontInfo Info = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size);
		return Info;
	}
}
