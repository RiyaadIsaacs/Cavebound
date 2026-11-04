#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

class UButton;
class UTexture2D;

/**
 * Prinbles Robin kit — dark wood counters/panels, cream button faces.
 * Textures live in Content/UI/Robin (@2x PNGs).
 */
namespace CaveboundUIStyle
{
	/** Cream face / icon disc. */
	inline FLinearColor Cream() { return FLinearColor(0.99f, 0.90f, 0.62f, 1.f); }
	/** Dark wood body. */
	inline FLinearColor Wood() { return FLinearColor(0.38f, 0.30f, 0.28f, 1.f); }
	/** Text on cream buttons. */
	inline FLinearColor InkOnCream() { return FLinearColor(0.35f, 0.27f, 0.24f, 1.f); }
	/** Text on wood counters/panels. */
	inline FLinearColor InkOnWood() { return Cream(); }
	inline FLinearColor Danger() { return FLinearColor(0.92f, 0.45f, 0.35f, 1.f); }

	// Back-compat aliases used by older call sites.
	inline FLinearColor Title() { return InkOnWood(); }
	inline FLinearColor Body() { return FLinearColor(0.96f, 0.86f, 0.58f, 1.f); }
	inline FLinearColor ButtonBg() { return Cream(); }
	inline FLinearColor ButtonBgActive() { return FLinearColor(1.f, 0.96f, 0.72f, 1.f); }
	inline FLinearColor PanelBg() { return Wood(); }
	inline FLinearColor Accent() { return FLinearColor(0.45f, 0.62f, 0.28f, 1.f); }
	inline FLinearColor HealthFill() { return Cream(); }
	inline FLinearColor HealthBack() { return Wood(); }
	inline FLinearColor BarFill() { return Cream(); }

	inline FSlateFontInfo Font(int32 Size)
	{
		return FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), Size);
	}

	UTexture2D* LoadRobinTexture(const TCHAR* FileName);

	FSlateBrush MakeBoxBrush(UTexture2D* Texture, const FMargin& BorderUV, const FVector2D& ImageSize);
	FSlateBrush MakeImageBrush(UTexture2D* Texture, const FVector2D& ImageSize);

	FButtonStyle MakeRectButtonStyle();
	FButtonStyle MakeSquareButtonStyle();

	void ApplyRectButtonStyle(UButton* Button);
	void ApplySquareButtonStyle(UButton* Button);

	/** Empty dark-wood counter plate (resource pills, keycaps). */
	FSlateBrush MakeCounterBrush();

	/** Same wood chrome, sized for larger menus / hover cards. */
	FSlateBrush MakePanelBrush();

	FSlateBrush MakeBarBackgroundBrush();
	FSlateBrush MakeBarFillBrush();
}
