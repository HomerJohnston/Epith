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

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		const FEpithWindowElement_HorizontalGroup_Slot& Slot = Slots[i];

		const FEpithSlotActions Actions = Context.IsDesigning() ? EpithDraw::MakeArraySlotActions(MutableSlots, i, StaticStruct()->GetDisplayNameText(), EEpithSlotMoveArrows::LeftRight) : FEpithSlotActions();

		TSharedPtr<SWidget> Widget = EpithDraw::DrawSlot(Slot.Child, ChildContext, Actions);

		if (!Widget.IsValid())
		{
			continue;
		}

		// The slot's own Padding setting is the only spacing (same as the Vertical Group)
		Box->AddSlot()
		.AutoWidth()
		[
			SNew(SBorder)
			.Padding(FMargin(Slot.DisplayParams.Padding) + FEpithStyle::GetBorderPadding(Slot.DisplayParams.Border, false))
			.BorderImage(FEpithStyle::GetBorderBrush(Slot.DisplayParams.Border, false))
			.BorderBackgroundColor(EpithColor::Noir)
			[
				Widget.ToSharedRef()
			]
		];
	}

	if (Context.IsDesigning())
	{
		// Fills the leftover width so the button sits at the right end of the row
		Box->AddSlot()
		.FillWidth(1.0f)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.Padding(4, 2)
		[
			Context.Designer->MakeAddButton(LOCTEXT("HorizontalGroup_Add", "Add Slot to Horizontal Group"), [SlotsPtr = &MutableSlots]()
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
