#include "Epith/Widgets/EpithWindowElement_HorizontalGroup.h"

#include "Epith/Style/EpithColor.h"
#include "Epith/Style/EpithStyle.h"
#include "Epith/Widgets/EpithDrawContext.h"

#define LOCTEXT_NAMESPACE "Epith"

TSharedPtr<SWidget> FEpithWindowElement_HorizontalGroup::Draw(const FEpithDrawContext& Context) const
{
	// In the designer, slots are edited in place (see EpithDraw::DrawSlot)
	TArray<FEpithWindowElement_HorizontalGroup_Slot>& MutableSlots = const_cast<TArray<FEpithWindowElement_HorizontalGroup_Slot>&>(Slots);
	
	// This group's property rows line up with each other
	const FEpithDrawContext ChildContext = Context.WithColumns(Columns);

	TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox);

	int32 SlotsNum = Slots.Num();

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		const FEpithWindowElement_HorizontalGroup_Slot& Slot = Slots[i];

		const FEpithSlotActions Actions = Context.IsDesigning() ? EpithDraw::MakeArraySlotActions(MutableSlots, i) : FEpithSlotActions();

		TSharedPtr<SWidget> Widget = EpithDraw::DrawSlot(Slot.Child, ChildContext, Actions);

		if (!Widget.IsValid())
		{
			continue;
		}

		Box->AddSlot()
		.AutoWidth()
		.Padding(4, 4, i == SlotsNum - 1 ? 4 : 0, 4)
		[
			SNew(SBorder)
			.Padding(Slot.DisplayParams.Padding)
			.BorderImage(Slot.DisplayParams.bBorder ? FEpithStyle::GetImageBrush(EpithBrushes.Border_2px_Deburred) : FAppStyle::GetBrush("NoBorder"))
			.BorderBackgroundColor(EpithColor::Noir)
			[
				Widget.ToSharedRef()
			]
		];
	}

	if (Context.IsDesigning())
	{
		Box->AddSlot()
		.AutoWidth()
		.Padding(4, 2)
		[
			Context.Designer->MakeAddButton(LOCTEXT("HorizontalGroup_Add", "Add"), [SlotsPtr = &MutableSlots]()
			{
				SlotsPtr->AddDefaulted();
			})
		];
	}

	return Box;
}

#undef LOCTEXT_NAMESPACE
