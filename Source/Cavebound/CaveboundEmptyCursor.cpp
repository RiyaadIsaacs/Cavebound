#include "CaveboundEmptyCursor.h"

#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"

void UCaveboundEmptyCursor::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("EmptyCursorSize"));
	Size->SetWidthOverride(1.f);
	Size->SetHeightOverride(1.f);
	Size->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = Size;
}
