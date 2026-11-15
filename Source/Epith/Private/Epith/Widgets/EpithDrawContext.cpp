#include "Epith/Widgets/EpithDrawContext.h"

#include "Epith/Widgets/EpithWindowElement.h"
#include "Widgets/SNullWidget.h"

TSharedPtr<SWidget> EpithDraw::DrawSlot(const TInstancedStruct<FEpithWindowElement>& Slot, const FEpithDrawContext& Context, const FEpithSlotActions& Actions)
{
	if (!Context.IsDesigning())
	{
		return Slot.IsValid() ? Slot.Get<FEpithWindowElement>().Draw(Context) : nullptr;
	}

	// The designer edits the panel definition's own layout data, which elements only see as const while drawing.
	// Every edit is followed by a redraw, so these references never outlive the data they point at.
	TInstancedStruct<FEpithWindowElement>& MutableSlot = const_cast<TInstancedStruct<FEpithWindowElement>&>(Slot);

	if (!Slot.IsValid())
	{
		return Context.Designer->MakeEmptySlot(MutableSlot, Actions);
	}

	TSharedPtr<SWidget> ElementWidget = Slot.Get<FEpithWindowElement>().Draw(Context);

	return Context.Designer->WrapElement(MutableSlot, ElementWidget.IsValid() ? ElementWidget.ToSharedRef() : SNullWidget::NullWidget, Actions);
}
