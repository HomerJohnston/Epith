#include "Epith/Widgets/EpithWindowElement_FoldoutGroup.h"

#include "Epith/Style/EpithColor.h"
#include "Epith/Widgets/EpithDrawContext.h"

TSharedPtr<SWidget> FEpithWindowElement_FoldoutGroup::Draw(const FEpithDrawContext& Context) const
{
	TSharedPtr<SWidget> ElementWidget = EpithDraw::DrawSlot(Element, Context.WithColumns(Columns));

	if (!ElementWidget.IsValid())
	{
		ElementWidget = SNew(STextBlock).Text(INVTEXT("No element set!"));
	}

	TSharedRef<SExpandableArea> ExpandableArea = SNew(SExpandableArea)
	.AreaTitle(FText::FromString(FoldoutTitle))
	.AllowAnimatedTransition(false)
	.BorderImage(FAppStyle::Get().GetBrush("DetailsView.CategoryTop"))
	.BorderBackgroundColor(EpithColor::DeepGray_BarelyTrans)
	.BodyContent()
	[
		ElementWidget.ToSharedRef()
	];

	return ExpandableArea;
}
