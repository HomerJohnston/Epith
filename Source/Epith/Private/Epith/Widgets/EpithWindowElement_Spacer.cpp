#include "Epith/Widgets/EpithWindowElement_Spacer.h"

#include "Epith/Widgets/EpithDrawContext.h"
#include "Widgets/Text/STextBlock.h"

#include "Styling/AppStyle.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "Epith"

TSharedPtr<SWidget> FEpithWindowElement_Spacer::Draw(const FEpithDrawContext& Context) const
{
	const bool bVertical = Context.ParentAxis == EEpithGroupAxis::Vertical;
	const bool bHorizontal = Context.ParentAxis == EEpithGroupAxis::Horizontal;

	// A spacer only means something between the slots of a Vertical or Horizontal Group
	if (!bVertical && !bHorizontal)
	{
		if (Context.IsDesigning())
		{
			return SNew(STextBlock)
				.Text(LOCTEXT("Spacer_WrongParent", "A Spacer only works in a Vertical or Horizontal Group."))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground());
		}

		return nullptr;
	}

	// The gap, with 4px of padding on both sides along the group's direction (above and below, or left and right)
	const FMargin Padding = bVertical ? FMargin(0.0f, PaddingAlongGroup) : FMargin(PaddingAlongGroup, 0.0f);

	// 8 x 8; the group stretches it the other way (full width in a Vertical Group, full height in a Horizontal Group)
	return SNew(SBox)
		.Padding(Padding)
		[
			SNew(SBox)
			.WidthOverride(Size)
			.HeightOverride(Size)
			[
				bFilled
					? StaticCastSharedRef<SWidget>(SNew(SImage).Image(FAppStyle::GetBrush("Brushes.Background")))
					: SNullWidget::NullWidget
			]
		];
}

#undef LOCTEXT_NAMESPACE
