#include "Epith/Widgets/EpithWindowElement_HorizontalGroup.h"

#include "Epith/Style/EpithColor.h"
#include "Epith/Style/EpithStyle.h"
#include "Epith/Widgets/EpithDrawContext.h"
#include "Epith/Widgets/EpithWindowElement_Divider.h"

#define LOCTEXT_NAMESPACE "Epith"

namespace EpithHorizontalGroup
{
	/**
	 * The slots' widths while the group is drawn: each slot's saved share of the group, which the designer changes live while a
	 * divider between two slots is dragged (saved when the drag ends).
	 */
	struct FSlotSizes
	{
		/** No slot gets less than this share (also the most a drag can shrink a slot to). */
		static constexpr float MinShare = 0.02f;

		/** Per slot: its share (0 = an equal part of what the others leave) */
		TArray<float> Fractions;

		/** Per slot: it takes only the width it needs (a divider), not a share */
		TArray<bool> bFixed;

		/** Per slot: its widget, to measure how wide the shared slots are */
		TArray<TWeakPtr<SWidget>> Widgets;

		// The drag in progress: the slot left of the divider (the one right of it is the next), and the shares when it began
		int32 DragLeft = INDEX_NONE;
		float StartLeft = 0.0f;
		float StartRight = 0.0f;
		float UnitsPerShare = 0.0f;

		/** The slot's part of the group's width, as a fill weight: its own share, or an equal part of what the others leave */
		float GetShare(int32 Index) const
		{
			float SetTotal = 0.0f;
			int32 NumUnset = 0;

			for (int32 i = 0; i < Fractions.Num(); ++i)
			{
				if (bFixed[i])
				{
					continue;
				}

				const float Fraction = FMath::Clamp(Fractions[i], 0.0f, 1.0f);

				if (Fraction > 0.0f)
				{
					SetTotal += Fraction;
				}
				else
				{
					++NumUnset;
				}
			}

			const float Fraction = FMath::Clamp(Fractions[Index], 0.0f, 1.0f);

			if (Fraction > 0.0f)
			{
				return Fraction;
			}

			return FMath::Max((1.0f - SetTotal) / FMath::Max(NumUnset, 1), MinShare);
		}

		bool CanResize(int32 Left) const
		{
			return Fractions.IsValidIndex(Left + 1) && !bFixed[Left] && !bFixed[Left + 1];
		}

		void BeginResize(int32 Left)
		{
			DragLeft = Left;

			// Both slots keep their current shares as set values, so neither they nor the other slots move when the drag begins
			StartLeft = GetShare(Left);
			StartRight = GetShare(Left + 1);

			// How many units of the group's width one share is: the shared slots' total width over their total share
			float SharedWidth = 0.0f;
			float SharedTotal = 0.0f;

			for (int32 i = 0; i < Fractions.Num(); ++i)
			{
				TSharedPtr<SWidget> Widget = Widgets[i].Pin();

				if (!bFixed[i] && Widget.IsValid())
				{
					SharedWidth += Widget->GetCachedGeometry().GetLocalSize().X;
					SharedTotal += GetShare(i);
				}
			}

			UnitsPerShare = SharedTotal > 0.0f ? SharedWidth / SharedTotal : 0.0f;

			Fractions[Left] = StartLeft;
			Fractions[Left + 1] = StartRight;
		}

		/** The boundary between the two slots is Delta units from where it was: one slot gains what the other loses */
		void UpdateResize(float Delta)
		{
			if (DragLeft == INDEX_NONE || UnitsPerShare <= 0.0f)
			{
				return;
			}

			const float Total = StartLeft + StartRight;
			float Left = FMath::Clamp(StartLeft + Delta / UnitsPerShare, MinShare, FMath::Min(1.0f, Total - MinShare));

			// Rounded to a whole percent, which is plenty to line things up by eye
			Left = FMath::RoundToFloat(Left * 100.0f) / 100.0f;

			Fractions[DragLeft] = Left;
			Fractions[DragLeft + 1] = FMath::Clamp(Total - Left, MinShare, 1.0f);
		}
	};
}

/** Dragging the divider between slot Left and the next one moves the boundary between them (designer only). */
static FEpithDividerResize MakeDividerResize(const TSharedRef<EpithHorizontalGroup::FSlotSizes>& Sizes, TArray<FEpithWindowElement_HorizontalGroup_Slot>& Slots, int32 Left)
{
	FEpithDividerResize Resize;

	Resize.CanResize = [Sizes, Left]()
	{
		return Sizes->CanResize(Left);
	};

	Resize.Begin = [Sizes, Left]()
	{
		Sizes->BeginResize(Left);
	};

	Resize.Update = [Sizes](float Delta)
	{
		Sizes->UpdateResize(Delta);
	};

	// The slot data stays put until the designer redraws after this edit
	Resize.Commit = [Sizes, SlotsPtr = &Slots, Left]()
	{
		if (SlotsPtr->IsValidIndex(Left + 1))
		{
			(*SlotsPtr)[Left].WidthFraction = FMath::Clamp(Sizes->Fractions[Left], 0.0f, 1.0f);
			(*SlotsPtr)[Left + 1].WidthFraction = FMath::Clamp(Sizes->Fractions[Left + 1], 0.0f, 1.0f);
		}

		Sizes->DragLeft = INDEX_NONE;
	};

	Resize.Cancel = [Sizes, SlotsPtr = &Slots, Left]()
	{
		if (SlotsPtr->IsValidIndex(Left + 1))
		{
			Sizes->Fractions[Left] = (*SlotsPtr)[Left].WidthFraction;
			Sizes->Fractions[Left + 1] = (*SlotsPtr)[Left + 1].WidthFraction;
		}

		Sizes->DragLeft = INDEX_NONE;
	};

	return Resize;
}

TSharedPtr<SWidget> FEpithWindowElement_HorizontalGroup::Draw(const FEpithDrawContext& Context) const
{
	// In the designer, slots are edited in place (see EpithDraw::DrawSlot)
	TArray<FEpithWindowElement_HorizontalGroup_Slot>& MutableSlots = const_cast<TArray<FEpithWindowElement_HorizontalGroup_Slot>&>(Slots);

	// This group's property rows line up with each other
	const FEpithDrawContext ChildContext = Context.WithColumns(Columns, EEpithGroupAxis::Horizontal);

	// Filled dividers join up across groups (see FEpithDrawContext::DividerReach). Not in the designer: its chrome sits between slots.
	const bool bJoinDividers = !Context.IsDesigning();
	const FMargin GroupBorderPadding = Border > 0 ? FEpithStyle::GetBorderPadding(Border, false) : FMargin(0.0f);

	// The space between a slot's edge and its content: its Padding setting plus its border
	auto GetSlotInset = [this](int32 Index)
	{
		const FEpithWindowElement_BoxDisplayParams& Params = Slots[Index].DisplayParams;
		return FMargin(Params.Padding) + FEpithStyle::GetBorderPadding(Params.Border, false);
	};

	TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox);

	// Every slot gets a share of the group's width, except dividers, which take only what they need
	TSharedRef<EpithHorizontalGroup::FSlotSizes> Sizes = MakeShared<EpithHorizontalGroup::FSlotSizes>();

	for (const FEpithWindowElement_HorizontalGroup_Slot& Slot : Slots)
	{
		Sizes->Fractions.Add(Slot.WidthFraction);
		Sizes->bFixed.Add(Slot.Child.GetPtr<FEpithWindowElement_Divider>() != nullptr);
		Sizes->Widgets.AddDefaulted();
	}

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		const FEpithWindowElement_HorizontalGroup_Slot& Slot = Slots[i];

		const FEpithSlotActions Actions = Context.IsDesigning() ? EpithDraw::MakeArraySlotActions(MutableSlots, i, StaticStruct()->GetDisplayNameText(), EEpithSlotReorder::Horizontal) : FEpithSlotActions();

		FEpithDrawContext SlotContext = ChildContext;

		// Property rows side by side get no top or bottom grid line (only the lines between their own columns)
		SlotContext.RowEdges = FEpithRowEdges();

		if (bJoinDividers)
		{
			const FMargin Inset = GetSlotInset(i);
			FMargin& Reach = SlotContext.DividerReach;

			// A divider bar beside this whole group: pass it on across the group to every slot, past this group's border and the slot's inset
			Reach.Top = Context.DividerReach.Top > 0.0f ? Context.DividerReach.Top + GroupBorderPadding.Top + Inset.Top : 0.0f;
			Reach.Bottom = Context.DividerReach.Bottom > 0.0f ? Context.DividerReach.Bottom + GroupBorderPadding.Bottom + Inset.Bottom : 0.0f;

			// A filled divider in the slot before or after this one: the distance from this slot's content to its bar
			if (i > 0 && EpithDraw::IsFilledDivider(Slots[i - 1].Child))
			{
				Reach.Left = FEpithWindowElement_Divider::PaddingAlongGroup + GetSlotInset(i - 1).Right + Inset.Left;
			}

			if (i < Slots.Num() - 1 && EpithDraw::IsFilledDivider(Slots[i + 1].Child))
			{
				Reach.Right = FEpithWindowElement_Divider::PaddingAlongGroup + GetSlotInset(i + 1).Left + Inset.Right;
			}
		}

		TSharedPtr<SWidget> Widget = EpithDraw::DrawSlot(Slot.Child, SlotContext, Actions);

		if (!Widget.IsValid())
		{
			continue;
		}

		// Designer: a divider between this slot and the previous one, which inserts a new slot there when clicked
		// Designer: a divider left of this slot (inserts a slot there when clicked; an element dragged onto it moves there).
		// Between two slots, dragging it sideways resizes them.
		if (Context.IsDesigning())
		{
			Box->AddSlot()
			.AutoWidth()
			[
				Context.Designer->MakeInsertDivider(false, LOCTEXT("HorizontalGroup_Insert", "Insert Slot into Horizontal Group"), [SlotsPtr = &MutableSlots, i]()
				{
					return &SlotsPtr->InsertDefaulted_GetRef(i).Child;
				},
				FEpithInsertPosition{ &MutableSlots, i },
				i > 0 ? MakeDividerResize(Sizes, MutableSlots, i - 1) : FEpithDividerResize())
			];
		}

		// The slot's own Padding setting is the only spacing (same as the Vertical Group)
		TSharedRef<SWidget> SlotWidget = SNew(SBorder)
			.Padding(FMargin(Slot.DisplayParams.Padding) + FEpithStyle::GetBorderPadding(Slot.DisplayParams.Border, false))
			.BorderImage(FEpithStyle::GetBorderBrush(Slot.DisplayParams.Border, false))
			.BorderBackgroundColor(EpithColor::Noir)
			[
				Widget.ToSharedRef()
			];

		Sizes->Widgets[i] = SlotWidget;

		if (Sizes->bFixed[i])
		{
			Box->AddSlot()
			.AutoWidth()
			[
				SlotWidget
			];
		}
		else
		{
			Box->AddSlot()
			.FillWidth(TAttribute<float>::CreateLambda([Sizes, i]()
			{
				return Sizes->GetShare(i);
			}))
			[
				SlotWidget
			];
		}
	}

	if (Context.IsDesigning())
	{
		// At the right end of the row (the slots share the rest of the width)
		Box->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(4, 2)
		[
			Context.Designer->MakeAddButton(LOCTEXT("HorizontalGroup_Add", "Add Slot to Horizontal Group"), [SlotsPtr = &MutableSlots]()
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
