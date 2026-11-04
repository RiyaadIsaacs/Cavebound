#include "CaveboundDamageVignette.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"

UTexture2D* UCaveboundDamageVignette::GetRadialTexture()
{
	static TWeakObjectPtr<UTexture2D> Cached;
	if (UTexture2D* Existing = Cached.Get())
	{
		return Existing;
	}

	// Soft radial mask: transparent in the middle, red toward the edges.
	const int32 Size = 512;
	UTexture2D* Texture = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
	if (!Texture)
	{
		return nullptr;
	}

	Texture->CompressionSettings = TextureCompressionSettings::TC_EditorIcon;
	Texture->LODGroup = TextureGroup::TEXTUREGROUP_UI;
	Texture->SRGB = true;
	Texture->Filter = TF_Bilinear;
	Texture->AddressX = TA_Clamp;
	Texture->AddressY = TA_Clamp;

	FTexturePlatformData* PlatformData = Texture->GetPlatformData();
	if (!PlatformData || PlatformData->Mips.Num() == 0)
	{
		return nullptr;
	}

	FTexture2DMipMap& Mip = PlatformData->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FColor* Pixels = static_cast<FColor*>(Data);

	const float Inv = 1.f / static_cast<float>(Size - 1);
	for (int32 Y = 0; Y < Size; ++Y)
	{
		for (int32 X = 0; X < Size; ++X)
		{
			const float U = X * Inv * 2.f - 1.f;
			const float V = Y * Inv * 2.f - 1.f;
			const float Dist = FMath::Sqrt(U * U + V * V);

			// Keep the centre clear; only the outer ring reads as vignette.
			float Alpha = FMath::SmoothStep(0.3f, 1.15f, Dist);
			Alpha = FMath::Pow(Alpha, 1.4f);

			Pixels[Y * Size + X] = FColor(190, 12, 12, static_cast<uint8>(FMath::Clamp(Alpha * 255.f, 0.f, 255.f)));
		}
	}

	Mip.BulkData.Unlock();
	Texture->UpdateResource();
	Cached = Texture;
	return Texture;
}

void UCaveboundDamageVignette::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::Collapsed);

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("VignetteCanvas"));
	Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = Canvas;

	VignetteImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("VignetteImage"));
	VignetteImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	VignetteImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));

	if (UTexture2D* Radial = GetRadialTexture())
	{
		VignetteImage->SetBrushFromTexture(Radial, true);
	}

	if (UCanvasPanelSlot* ImageSlot = Canvas->AddChildToCanvas(VignetteImage))
	{
		// Stretch one radial image across the whole viewport.
		ImageSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		ImageSlot->SetOffsets(FMargin(0.f));
	}
}

void UCaveboundDamageVignette::SetStrength(float Alpha)
{
	if (VignetteImage)
	{
		VignetteImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f)));
	}
}

void UCaveboundDamageVignette::Pulse(float Strength, float InDuration)
{
	PeakStrength = FMath::Clamp(Strength, 0.f, 1.f);
	Duration = FMath::Max(0.05f, InDuration);
	Life = Duration;
	bActive = true;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetStrength(PeakStrength);
}

void UCaveboundDamageVignette::TickFade(float DeltaTime)
{
	if (!bActive)
	{
		return;
	}

	Life -= DeltaTime;
	if (Life <= 0.f)
	{
		bActive = false;
		SetStrength(0.f);
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const float T = Life / Duration;
	// Hard hit, then ease out.
	SetStrength(PeakStrength * FMath::InterpEaseIn(0.f, 1.f, T, 2.f));
}
