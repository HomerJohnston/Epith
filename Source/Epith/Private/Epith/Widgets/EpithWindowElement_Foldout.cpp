#include "Epith/Widgets/EpithWindowElement_Foldout.h"

#include "Epith/Style/EpithColor.h"
#include "Epith/Style/EpithStyle.h"
#include "Epith/Widgets/EpithDrawContext.h"

TSharedPtr<SWidget> FEpithWindowElement_Foldout::Draw(const FEpithDrawContext& Context) const
{
	TSharedPtr<SWidget> ElementWidget = EpithDraw::DrawSlot(Element, Context.WithColumns(Columns));

	if (!ElementWidget.IsValid())
	{
		ElementWidget = SNew(STextBlock).Text(INVTEXT("No element set!"));
	}

	// A U-shaped border: the foldout bar closes it at the top
	if (Border > 0)
	{
		ElementWidget = SNew(SBorder)
			.BorderImage(FEpithStyle::GetBorderBrush(Border, true))
			.BorderBackgroundColor(EpithColor::Noir_SemiTrans)
			.Padding(FEpithStyle::GetBorderPadding(Border, true))
			[
				ElementWidget.ToSharedRef()
			];
	}

	// Keep it open or closed after a redraw
	const void* StateKey = this;
	TWeakPtr<FEpithViewState> WeakViewState = Context.ViewState;
	const bool* SavedExpanded = Context.ViewState.IsValid() ? Context.ViewState->FoldoutExpanded.Find(StateKey) : nullptr;

	bool bExpanded = true;

	if (SavedExpanded)
	{
		bExpanded = *SavedExpanded;
	}
	else
	{
		// The first time it's shown (the panel just opened): folded, unless something inside has been changed from its default,
		// so the changes on this object are what you see first. Always open in the designer, where it's being worked on.
		bExpanded = Context.IsDesigning() || EpithDraw::HasModifiedProperty(Element, Context);

		// Remembered, so a later redraw doesn't fold it (or pop it open) as values change
		if (Context.ViewState.IsValid())
		{
			Context.ViewState->FoldoutExpanded.Add(StateKey, bExpanded);
		}
	}

	TSharedRef<SExpandableArea> ExpandableArea = SNew(SExpandableArea)
	.AreaTitle(FText::FromString(FoldoutTitle))
	.InitiallyCollapsed(!bExpanded)
	.OnAreaExpansionChanged_Lambda([WeakViewState, StateKey](bool bNowExpanded)
	{
		if (TSharedPtr<FEpithViewState> ViewState = WeakViewState.Pin())
		{
			ViewState->FoldoutExpanded.Add(StateKey, bNowExpanded);
		}
	})
	.AllowAnimatedTransition(false)
	// SExpandableArea pads its body by 1px by default; the contents (and their border) go right to the edge
	.Padding(0.0f)
	.BorderImage(FAppStyle::Get().GetBrush("DetailsView.CategoryTop"))
	.BorderBackgroundColor(EpithColor::DeepGray_BarelyTrans)
	.BodyContent()
	[
		ElementWidget.ToSharedRef()
	];

	return ExpandableArea;
}

void FEpithWindowElement_Foldout::PostSerialize(const FArchive& Ar)
{
	if (Ar.IsLoading() && bBorder_DEPRECATED)
	{
		Border = 1;
		bBorder_DEPRECATED = false;
	}
}
