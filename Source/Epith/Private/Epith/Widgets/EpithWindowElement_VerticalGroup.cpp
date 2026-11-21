#include "Epith/Widgets/EpithWindowElement_VerticalGroup.h"

#include "Epith/Style/EpithColor.h"
#include "Epith/Style/EpithStyle.h"
#include "Epith/Widgets/EpithDrawContext.h"
#include "Epith/Widgets/EpithWindowElement_Property.h"
#include "Epith/Widgets/EpithWindowElement_Divider.h"

#define LOCTEXT_NAMESPACE "Epith"

TSharedPtr<SWidget> FEpithWindowElement_VerticalGroup::Draw(const FEpithDrawContext& Context) const
{
	// In the designer, slots are edited in place (see EpithDraw::DrawSlot)
	TArray<FEpithWindowElement_VerticalGroup_Slot>& MutableSlots = const_cast<TArray<FEpithWindowElement_VerticalGroup_Slot>&>(Slots);

	// This group's property rows line up with each other
	const FEpithDrawContext ChildContext = Context.WithColumns(Columns, EEpithGroupAxis::Vertical);

	// Filled dividers join up across groups (see FEpithDrawContext::DividerReach). Not in the designer: its chrome sits between slots.
	const bool bJoinDividers = !Context.IsDesigning();
	const FMargin GroupBorderPadding = Border > 0 ? FEpithStyle::GetBorderPadding(Border, false) : FMargin(0.0f);

	// The space between a slot's edge and its content: its Padding setting plus its border
	auto GetSlotInset = [this](int32 Index)
	{
		const FEpithWindowElement_BoxDisplayParams& Params = Slots[Index].DisplayParams;
		return FMargin(Params.Padding) + FEpithStyle::GetBorderPadding(Params.Border, false);
	};

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	// The previous slot's widget, if it holds a Property element whose row sits flush against this slot (no padding or border between)
	TWeakPtr<SWidget> RowAbove;

	// That slot's own top-line rule: if its row is hidden (e.g. by a customization), the row below takes the line in its place
	TAttribute<bool> TopLineAbove;

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		const FEpithWindowElement_VerticalGroup_Slot& Slot = Slots[i];

		const FEpithSlotActions Actions = Context.IsDesigning() ? EpithDraw::MakeArraySlotActions(MutableSlots, i, StaticStruct()->GetDisplayNameText(), EEpithSlotReorder::Vertical) : FEpithSlotActions();

		FEpithDrawContext SlotContext = ChildContext;

		if (bJoinDividers)
		{
			const FMargin Inset = GetSlotInset(i);
			FMargin& Reach = SlotContext.DividerReach;

			// A divider bar beside this whole group: pass it on across the group to every slot, past this group's border and the slot's inset
			Reach.Left = Context.DividerReach.Left > 0.0f ? Context.DividerReach.Left + GroupBorderPadding.Left + Inset.Left : 0.0f;
			Reach.Right = Context.DividerReach.Right > 0.0f ? Context.DividerReach.Right + GroupBorderPadding.Right + Inset.Right : 0.0f;

			// A filled divider in the slot before or after this one: the distance from this slot's content to its bar
			if (i > 0 && EpithDraw::IsFilledDivider(Slots[i - 1].Child))
			{
				Reach.Top = FEpithWindowElement_Divider::PaddingAlongGroup + GetSlotInset(i - 1).Bottom + Inset.Top;
			}

			if (i < Slots.Num() - 1 && EpithDraw::IsFilledDivider(Slots[i + 1].Child))
			{
				Reach.Bottom = FEpithWindowElement_Divider::PaddingAlongGroup + GetSlotInset(i + 1).Top + Inset.Bottom;
			}
		}

		// Property rows here get a bottom grid line, and a top one unless a visible property row sits directly above
		SlotContext.RowEdges.bBottom = true;
		SlotContext.RowEdges.Top = TAttribute<bool>::CreateLambda([RowAbove, TopLineAbove]()
		{
			TSharedPtr<SWidget> Above = RowAbove.Pin();

			if (!Above.IsValid())
			{
				return true;
			}

			// A hidden row above: this row is now the one meeting whatever is above that, so it follows the hidden row's rule
			return Above->GetVisibility() == EVisibility::Collapsed ? TopLineAbove.Get(true) : false;
		});

		TSharedPtr<SWidget> Widget = EpithDraw::DrawSlot(Slot.Child, SlotContext, Actions);

		const bool bFlushBelow = i < Slots.Num() - 1 && GetSlotInset(i).Bottom <= 0.0f && GetSlotInset(i + 1).Top <= 0.0f;
		RowAbove = Widget.IsValid() && bFlushBelow && Slot.Child.GetPtr<FEpithWindowElement_Property>() ? Widget : nullptr;
		TopLineAbove = SlotContext.RowEdges.Top;

		if (!Widget.IsValid())
		{
			continue;
		}

		// Designer: a divider above this slot, which inserts a new slot there when clicked (an element dragged onto it moves there)
		if (Context.IsDesigning())
		{
			Box->AddSlot()
			.AutoHeight()
			[
				Context.Designer->MakeInsertDivider(true, LOCTEXT("VerticalGroup_Insert", "Insert Slot into Vertical Group"), [SlotsPtr = &MutableSlots, i]()
				{
					return &SlotsPtr->InsertDefaulted_GetRef(i).Child;
				},
				FEpithInsertPosition{ &MutableSlots, i })
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
				return &SlotsPtr->AddDefaulted_GetRef().Child;
			},
			FEpithInsertPosition{ &MutableSlots, MutableSlots.Num() })
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
