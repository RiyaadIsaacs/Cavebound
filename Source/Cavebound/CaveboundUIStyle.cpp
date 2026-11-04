#include "CaveboundUIStyle.h"

#include "Components/Button.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"

namespace CaveboundUIStyle
{
	static TMap<FName, TWeakObjectPtr<UTexture2D>> GTextureCache;

	UTexture2D* LoadRobinTexture(const TCHAR* FileName)
	{
		const FName Key(FileName);
		if (UTexture2D* Cached = GTextureCache.FindRef(Key).Get())
		{
			return Cached;
		}

		const FString Path = FPaths::ProjectContentDir() / TEXT("UI/Robin") / FileName;
		UTexture2D* Texture = FImageUtils::ImportFileAsTexture2D(Path);
		if (!Texture)
		{
			UE_LOG(LogTemp, Warning, TEXT("CaveboundUIStyle: failed to load %s"), *Path);
			return nullptr;
		}

		// Keep crisp UI pixels — no mips, UI group, unfiltered compression.
		Texture->CompressionSettings = TC_EditorIcon;
		Texture->LODGroup = TEXTUREGROUP_UI;
		Texture->MipGenSettings = TMGS_NoMipmaps;
		Texture->SRGB = true;
		Texture->Filter = TF_Bilinear;
		Texture->UpdateResource();
		Texture->AddToRoot();

		GTextureCache.Add(Key, Texture);
		return Texture;
	}

	FSlateBrush MakeBoxBrush(UTexture2D* Texture, const FMargin& BorderUV, const FVector2D& ImageSize)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = FSlateColor(FLinearColor::White);
		Brush.Margin = BorderUV;
		Brush.ImageSize = ImageSize;
		if (Texture)
		{
			Brush.SetResourceObject(Texture);
		}
		return Brush;
	}

	FSlateBrush MakeImageBrush(UTexture2D* Texture, const FVector2D& ImageSize)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.TintColor = FSlateColor(FLinearColor::White);
		Brush.ImageSize = ImageSize;
		if (Texture)
		{
			Brush.SetResourceObject(Texture);
		}
		return Brush;
	}

	FButtonStyle MakeRectButtonStyle()
	{
		UTexture2D* DefaultTex = LoadRobinTexture(TEXT("BTN_Rect_Default.png"));
		UTexture2D* HoverTex = LoadRobinTexture(TEXT("BTN_Rect_Hover.png"));

		// Slate Box margins are Margin * ACTUAL texture pixels (not ImageSize).
		// @2x rect is 392x168 — keep a ~20px wood frame so the cream face can stretch.
		const float TexW = DefaultTex ? static_cast<float>(DefaultTex->GetSizeX()) : 392.f;
		const float TexH = DefaultTex ? static_cast<float>(DefaultTex->GetSizeY()) : 168.f;
		const FMargin Border(20.f / TexW, 20.f / TexH);
		const FVector2D Size(220.f, 72.f);

		FButtonStyle Style;
		Style.SetNormal(MakeBoxBrush(DefaultTex, Border, Size));
		Style.SetHovered(MakeBoxBrush(HoverTex ? HoverTex : DefaultTex, Border, Size));
		Style.SetPressed(MakeBoxBrush(HoverTex ? HoverTex : DefaultTex, Border, Size));
		Style.SetDisabled(MakeBoxBrush(DefaultTex, Border, Size));
		Style.NormalPadding = FMargin(28.f, 14.f);
		Style.PressedPadding = FMargin(28.f, 16.f, 28.f, 12.f);
		return Style;
	}

	FButtonStyle MakeSquareButtonStyle()
	{
		UTexture2D* DefaultTex = LoadRobinTexture(TEXT("BTN_Square_Default.png"));
		UTexture2D* HoverTex = LoadRobinTexture(TEXT("BTN_Square_Hover.png"));
		// Draw as Image so chamfers stay intact; don't 9-slice squares.
		const FVector2D Size(64.f, 64.f);

		FButtonStyle Style;
		Style.SetNormal(MakeImageBrush(DefaultTex, Size));
		Style.SetHovered(MakeImageBrush(HoverTex ? HoverTex : DefaultTex, Size));
		Style.SetPressed(MakeImageBrush(HoverTex ? HoverTex : DefaultTex, Size));
		Style.SetDisabled(MakeImageBrush(DefaultTex, Size));
		Style.NormalPadding = FMargin(8.f);
		Style.PressedPadding = FMargin(8.f, 10.f, 8.f, 6.f);
		return Style;
	}

	void ApplyRectButtonStyle(UButton* Button)
	{
		if (Button)
		{
			Button->SetStyle(MakeRectButtonStyle());
			Button->SetBackgroundColor(FLinearColor::White);
			Button->SetColorAndOpacity(FLinearColor::White);
		}
	}

	void ApplySquareButtonStyle(UButton* Button)
	{
		if (Button)
		{
			Button->SetStyle(MakeSquareButtonStyle());
			Button->SetBackgroundColor(FLinearColor::White);
			Button->SetColorAndOpacity(FLinearColor::White);
		}
	}

	FSlateBrush MakeCounterBrush()
	{
		UTexture2D* Tex = LoadRobinTexture(TEXT("PANEL_Counter.png"));
		const float TexW = Tex ? static_cast<float>(Tex->GetSizeX()) : 288.f;
		const float TexH = Tex ? static_cast<float>(Tex->GetSizeY()) : 120.f;
		return MakeBoxBrush(Tex, FMargin(18.f / TexW, 18.f / TexH), FVector2D(168.f, 44.f));
	}

	FSlateBrush MakePanelBrush()
	{
		UTexture2D* Tex = LoadRobinTexture(TEXT("PANEL_Counter.png"));
		const float TexW = Tex ? static_cast<float>(Tex->GetSizeX()) : 288.f;
		const float TexH = Tex ? static_cast<float>(Tex->GetSizeY()) : 120.f;
		return MakeBoxBrush(Tex, FMargin(16.f / TexW, 16.f / TexH), FVector2D(320.f, 140.f));
	}

	FSlateBrush MakeBarBackgroundBrush()
	{
		UTexture2D* Tex = LoadRobinTexture(TEXT("BAR_Background.png"));
		const float TexW = Tex ? static_cast<float>(Tex->GetSizeX()) : 992.f;
		const float TexH = Tex ? static_cast<float>(Tex->GetSizeY()) : 104.f;
		return MakeBoxBrush(Tex, FMargin(26.f / TexW, 26.f / TexH), FVector2D(240.f, 28.f));
	}

	FSlateBrush MakeBarFillBrush()
	{
		UTexture2D* Tex = LoadRobinTexture(TEXT("BAR_Fill.png"));
		const float TexW = Tex ? static_cast<float>(Tex->GetSizeX()) : 960.f;
		const float TexH = Tex ? static_cast<float>(Tex->GetSizeY()) : 64.f;
		return MakeBoxBrush(Tex, FMargin(18.f / TexW, 18.f / TexH), FVector2D(220.f, 18.f));
	}
}
