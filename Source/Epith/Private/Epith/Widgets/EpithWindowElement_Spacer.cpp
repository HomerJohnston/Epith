#include "Epith/Widgets/EpithWindowElement_Spacer.h"

#include "Epith/Widgets/EpithDrawContext.h"
#include "Widgets/Text/STextBlock.h"

#include "Styling/AppStyle.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SLeafWidget.h"

#define LOCTEXT_NAMESPACE "Epith"

namespace EpithSpacer
{
	/**
	 * A filled spacer's bar. Paints past its own ends by Reach to meet a neighbouring group's spacer bar. It only paints further;
	 * its size, and so the layout, stay the same.
	 */
	class SBar : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SBar) {}
			SLATE_ARGUMENT(FMargin, Reach)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Reach = InArgs._Reach;
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(FEpithWindowElement_Spacer::Size, FEpithWindowElement_Spacer::Size);
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			const FSlateBrush* Brush = FAppStyle::GetBrush("Brushes.Background");
			const FVector2f LocalSize = AllottedGeometry.GetLocalSize();

			const FVector2f PaintPosition(-Reach.Left, -Reach.Top);
			const FVector2f PaintSize(LocalSize.X + Reach.Left + Reach.Right, LocalSize.Y + Reach.Top + Reach.Bottom);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(PaintSize, FSlateLayoutTransform(PaintPosition)),
				Brush,
				ESlateDrawEffect::None,
				InWidgetStyle.GetColorAndOpacityTint() * Brush->GetTint(InWidgetStyle));

			return LayerId;
		}

	private:
		FMargin Reach;
	};
}

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

	// The bar only reaches along its length: sideways for a Vertical Group's (horizontal) bar, up and down for a Horizontal Group's
	const FMargin Reach = bVertical
		? FMargin(Context.SpacerReach.Left, 0.0f, Context.SpacerReach.Right, 0.0f)
		: FMargin(0.0f, Context.SpacerReach.Top, 0.0f, Context.SpacerReach.Bottom);

	// 8 x 8; the group stretches it the other way (full width in a Vertical Group, full height in a Horizontal Group)
	return SNew(SBox)
		.Padding(Padding)
		[
			SNew(SBox)
			.WidthOverride(Size)
			.HeightOverride(Size)
			[
				bFilled
					? StaticCastSharedRef<SWidget>(SNew(EpithSpacer::SBar).Reach(Reach))
					: SNullWidget::NullWidget
			]
		];
}

#undef LOCTEXT_NAMESPACE
