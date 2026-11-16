#include "Epith/Widgets/EpithWindowElement_Spacer.h"

#include "Styling/AppStyle.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"

TSharedPtr<SWidget> FEpithWindowElement_Spacer::Draw(const FEpithDrawContext& Context) const
{
	// 8 x 8; the group stretches it the other way (full width in a Vertical Group, full height in a Horizontal Group)
	return SNew(SBox)
		.WidthOverride(Size)
		.HeightOverride(Size)
		[
			bFilled
				? StaticCastSharedRef<SWidget>(SNew(SImage).Image(FAppStyle::GetBrush("Brushes.Background")))
				: SNullWidget::NullWidget
		];
}
