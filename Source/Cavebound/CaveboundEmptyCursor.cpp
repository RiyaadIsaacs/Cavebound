#include "CaveboundEmptyCursor.h"

#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"

void UCaveboundEmptyCursor::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// invisible software cursor so the OS arrow can be replaced
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	// 1px hitless root — cursor shows nothing visually
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("EmptyCursorSize"));
	Size->SetWidthOverride(1.f);
	Size->SetHeightOverride(1.f);
	Size->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = Size;
}
