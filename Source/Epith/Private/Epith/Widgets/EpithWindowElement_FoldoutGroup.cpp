#include "Epith/Widgets/EpithWindowElement_FoldoutGroup.h"

TSharedPtr<SWidget> FEpithWindowElement_FoldoutGroup::Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const
{
	TSharedPtr<SWidget> ElementWidget = nullptr;
	
	if (Element.IsValid())
	{
		ElementWidget = Element.Get<FEpithWindowElement>().Draw(InPropertyData);
	}
	else
	{
		ElementWidget = SNew(STextBlock).Text(INVTEXT("No element set!"));
	}
	
	TSharedRef<SExpandableArea> ExpandableArea = SNew(SExpandableArea)
	.AreaTitle(FText::FromString(FoldoutTitle))
		.AllowAnimatedTransition(false)
		.BorderImage(FAppStyle::Get().GetBrush("DetailsView.CategoryTop"))
	.BodyContent()
	[
		ElementWidget.ToSharedRef()
	];
	
	return ExpandableArea;
}
