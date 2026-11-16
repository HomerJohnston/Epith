#include "Epith/Widgets/EpithWindowElement_HorizontalGroup.h"

#include "Epith/Style/EpithColor.h"
#include "Epith/Style/EpithStyle.h"
#include "Epith/Widgets/EpithDrawContext.h"
#include "Epith/Widgets/EpithWindowElement_Spacer.h"

#define LOCTEXT_NAMESPACE "Epith"

TSharedPtr<SWidget> FEpithWindowElement_HorizontalGroup::Draw(const FEpithDrawContext& Context) const
{
	// In the designer, slots are edited in place (see EpithDraw::DrawSlot)
	TArray<FEpithWindowElement_HorizontalGroup_Slot>& MutableSlots = const_cast<TArray<FEpithWindowElement_HorizontalGroup_Slot>&>(Slots);

	// This group's property rows line up with each other
	const FEpithDrawContext ChildContext = Context.WithColumns(Columns, EEpithGroupAxis::Horizontal);

	// Filled spacers join up across groups (see FEpithDrawContext::SpacerReach). Not in the designer: its chrome sits between slots.
	const bool bJoinSpacers = !Context.IsDesigning();
	const FMargin GroupBorderPadding = Border > 0 ? FEpithStyle::GetBorderPadding(Border, false) : FMargin(0.0f);

	// The space between a slot's edge and its content: its Padding setting plus its border
	auto GetSlotInset = [this](int32 Index)
	{
		const FEpithWindowElement_BoxDisplayParams& Params = Slots[Index].DisplayParams;
		return FMargin(Params.Padding) + FEpithStyle::GetBorderPadding(Params.Border, false);
	};

	TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox);

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		const FEpithWindowElement_HorizontalGroup_Slot& Slot = Slots[i];

		const FEpithSlotActions Actions = Context.IsDesigning() ? EpithDraw::MakeArraySlotActions(MutableSlots, i, StaticStruct()->GetDisplayNameText(), EEpithSlotReorder::Horizontal) : FEpithSlotActions();

		FEpithDrawContext SlotContext = ChildContext;

		// Property rows side by side get no top or bottom grid line (only the lines between their own columns)
		SlotContext.RowEdges = FEpithRowEdges();

		if (bJoinSpacers)
		{
			const FMargin Inset = GetSlotInset(i);
			FMargin& Reach = SlotContext.SpacerReach;

			// A spacer bar beside this whole group: pass it on across the group to every slot, past this group's border and the slot's inset
			Reach.Top = Context.SpacerReach.Top > 0.0f ? Context.SpacerReach.Top + GroupBorderPadding.Top + Inset.Top : 0.0f;
			Reach.Bottom = Context.SpacerReach.Bottom > 0.0f ? Context.SpacerReach.Bottom + GroupBorderPadding.Bottom + Inset.Bottom : 0.0f;

			// A filled spacer in the slot before or after this one: the distance from this slot's content to its bar
			if (i > 0 && EpithDraw::IsFilledSpacer(Slots[i - 1].Child))
			{
				Reach.Left = FEpithWindowElement_Spacer::PaddingAlongGroup + GetSlotInset(i - 1).Right + Inset.Left;
			}

			if (i < Slots.Num() - 1 && EpithDraw::IsFilledSpacer(Slots[i + 1].Child))
			{
				Reach.Right = FEpithWindowElement_Spacer::PaddingAlongGroup + GetSlotInset(i + 1).Left + Inset.Right;
			}
		}

		TSharedPtr<SWidget> Widget = EpithDraw::DrawSlot(Slot.Child, SlotContext, Actions);

		if (!Widget.IsValid())
		{
			continue;
		}

		// Designer: a divider between this slot and the previous one, which inserts a new slot there when clicked
		if (Context.IsDesigning() && i > 0)
		{
			Box->AddSlot()
			.AutoWidth()
			[
				Context.Designer->MakeInsertDivider(false, LOCTEXT("HorizontalGroup_Insert", "Insert Slot into Horizontal Group"), [SlotsPtr = &MutableSlots, i]()
				{
					SlotsPtr->InsertDefaulted(i);
				})
			];
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
