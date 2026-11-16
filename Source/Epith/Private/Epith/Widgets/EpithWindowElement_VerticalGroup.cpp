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
	const FEpithDrawContext ChildContext = Context.WithColumns(Columns, EEpithGroupAxis::Vertical);

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		const FEpithWindowElement_VerticalGroup_Slot& Slot = Slots[i];

		const FEpithSlotActions Actions = Context.IsDesigning() ? EpithDraw::MakeArraySlotActions(MutableSlots, i, StaticStruct()->GetDisplayNameText(), EEpithSlotReorder::Vertical) : FEpithSlotActions();

		TSharedPtr<SWidget> Widget = EpithDraw::DrawSlot(Slot.Child, ChildContext, Actions);

		if (!Widget.IsValid())
		{
			continue;
		}

		// Designer: a divider between this slot and the previous one, which inserts a new slot there when clicked
		if (Context.IsDesigning() && i > 0)
		{
			Box->AddSlot()
			.AutoHeight()
			[
				Context.Designer->MakeInsertDivider(true, LOCTEXT("VerticalGroup_Insert", "Insert Slot into Vertical Group"), [SlotsPtr = &MutableSlots, i]()
				{
					SlotsPtr->InsertDefaulted(i);
				})
			];
		}

		// The slot's own Padding setting is the only spacing, so property rows can sit flush like a Details panel
		Box->AddSlot()
		.AutoHeight()
		[
			SNew(SBorder)
			.Padding(FMargin(Slot.DisplayParams.Padding) + FEpithStyle::GetBorderPadding(Slot.DisplayParams.Border, false))
			.BorderImage(FEpithStyle::GetBorderBrush(Slot.DisplayParams.Border, false))
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
		.HAlign(HAlign_Right)
		.Padding(4, 2)
		[
			Context.Designer->MakeAddButton(LOCTEXT("VerticalGroup_Add", "Add Slot to Vertical Group"), [SlotsPtr = &MutableSlots]()
			{
				SlotsPtr->AddDefaulted();
			})
		];
	}

	if (Border == 0)
	{
		return Box;
	}

	// The group's own border, around all of its slots
	return SNew(SBorder)
		.BorderImage(FEpithStyle::GetBorderBrush(Border, false))
		.BorderBackgroundColor(EpithColor::Noir_SemiTrans)
		.Padding(FEpithStyle::GetBorderPadding(Border, false))
		[
			Box
		];
}

#undef LOCTEXT_NAMESPACE
