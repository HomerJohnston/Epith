#include "Epith/Widgets/EpithWindowElement_EmptySpace.h"

#include "Epith/Widgets/EpithDrawContext.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "Epith"

TSharedPtr<SWidget> FEpithWindowElement_EmptySpace::Draw(const FEpithDrawContext& Context) const
{
	// In the designer, a faint label so the slot can be seen and clicked; the real panel (and REAL VIEW) draws nothing
	if (Context.IsDesigning())
	{
		return SNew(SBox)
			.MinDesiredHeight(20.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("EmptySpace_Placeholder", "Spacer"))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];
	}

	return SNew(SSpacer);
}

#undef LOCTEXT_NAMESPACE
