#include "Epith/Widgets/EpithWindowElement_VerticalGroup.h"

#include "Epith/Style/EpithColor.h"
#include "Epith/Style/EpithStyle.h"
#include "Epith/Widgets/EpithDrawContext.h"

#define LOCTEXT_NAMESPACE "Epith"

TSharedPtr<SWidget> FEpithWindowElement_VerticalGroup::Draw(const FEpithDrawContext& Context) const
{
	// In the designer, slots are edited in place (see EpithDraw::DrawSlot)
	TArray<FEpithWindowElement_VerticalGroup_Slot>& MutableSlots = const_cast<TArray<FEpithWindowElement_VerticalGroup_Slot>&>(Slots);

	// This group's property rows line up with each other
	const FEpithDrawContext ChildContext = Context.WithColumns(Columns);

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		const FEpithWindowElement_VerticalGroup_Slot& Slot = Slots[i];

		const FEpithSlotActions Actions = Context.IsDesigning() ? EpithDraw::MakeArraySlotActions(MutableSlots, i) : FEpithSlotActions();

		TSharedPtr<SWidget> Widget = EpithDraw::DrawSlot(Slot.Child, ChildContext, Actions);

		if (!Widget.IsValid())
		{
			continue;
		}

		// The slot's own Padding setting is the only spacing, so property rows can sit flush like a Details panel
		Box->AddSlot()
		.AutoHeight()
		[
			SNew(SBorder)
			.Padding(Slot.DisplayParams.Padding)
			.BorderImage(Slot.DisplayParams.bBorder ? FEpithStyle::GetImageBrush(EpithBrushes.Border_2px_Deburred) : FAppStyle::GetBrush("NoBorder"))
			.BorderBackgroundColor(EpithColor::Noir_SemiTrans)
			[
				Widget.ToSharedRef()
			]
		];
	}

	if (Context.IsDesigning())
	{
		Box->AddSlot()
		.AutoHeight()
		.Padding(4, 2)
		[
			Context.Designer->MakeAddButton(LOCTEXT("VerticalGroup_Add", "+ to Vertical Group"), [SlotsPtr = &MutableSlots]()
			{
				SlotsPtr->AddDefaulted();
			})
		];
	}

	return Box;
}

#undef LOCTEXT_NAMESPACE
