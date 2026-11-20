#include "Epith/Widgets/EpithWindowElement_TabbedGroup.h"

#include "Epith/Widgets/EpithDrawContext.h"
#include "Epith/Widgets/SEpithTabbedPane.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "Epith"

TSharedPtr<SWidget> FEpithWindowElement_TabbedGroup::Draw(const FEpithDrawContext& Context) const
{
	// In the designer, slots are edited in place (see EpithDraw::DrawSlot)
	TArray<FEpithWindowElement_TabbedGroup_Slot>& MutableSlots = const_cast<TArray<FEpithWindowElement_TabbedGroup_Slot>&>(Slots);

	// All tabs share one column layout, so switching tabs doesn't make the columns jump
	const FEpithDrawContext ChildContext = Context.WithColumns(Columns);

	TArray<FText> Tabs;
	TArray<TSharedRef<SWidget>> Panes;

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		FText TabText = FText::FromString(Slots[i].TabName);

		if (TabText.IsEmpty())
		{
			TabText = FText::Format(LOCTEXT("TabbedGroup_DefaultTabName", "Tab {0}"), i + 1);
		}

		const FEpithSlotActions Actions = Context.IsDesigning() ? EpithDraw::MakeArraySlotActions(MutableSlots, i, StaticStruct()->GetDisplayNameText()) : FEpithSlotActions();

		TSharedPtr<SWidget> PaneWidget = EpithDraw::DrawSlot(Slots[i].Element, ChildContext, Actions);

		if (!PaneWidget.IsValid())
		{
			continue;
		}

		Tabs.Add(TabText);
		Panes.Add(PaneWidget.ToSharedRef());
	}

	// Designer: an add button to the right of the tabs (or on its own while there are no tabs yet)
	TSharedPtr<SWidget> AddButton;

	if (Context.IsDesigning())
	{
		AddButton = Context.Designer->MakeAddButton(LOCTEXT("TabbedGroup_AddTab", "Add Slot to Tabbed Group"), [SlotsPtr = &MutableSlots]()
		{
			FEpithWindowElement_TabbedGroup_Slot& NewSlot = SlotsPtr->AddDefaulted_GetRef();
			NewSlot.TabName = FString::Printf(TEXT("Tab %d"), SlotsPtr->Num());
			return &NewSlot.Element;
		});
	}

	if (Panes.Num() > 0)
	{
		// The gap between the tabs and the add button belongs to the button, so the real panel's tabs reach the right edge
		TSharedPtr<SWidget> TabBarExtra;

		if (AddButton.IsValid())
		{
			TabBarExtra = SNew(SBox)
				.Padding(4, 0, 0, 0)
				[
					AddButton.ToSharedRef()
				];
		}

		// Keep showing the same tab after a redraw
		const void* StateKey = this;
		TWeakPtr<FEpithViewState> WeakViewState = Context.ViewState;
		const int32* SavedIndex = Context.ViewState.IsValid() ? Context.ViewState->TabIndices.Find(StateKey) : nullptr;

		return SNew(SEpithTabbedPane)
			.Border(Border)
			.InitialIndex(SavedIndex ? static_cast<uint8>(FMath::Clamp(*SavedIndex, 0, 255)) : 0)
			.OnTabChanged([WeakViewState, StateKey](uint8 NewIndex)
			{
				if (TSharedPtr<FEpithViewState> ViewState = WeakViewState.Pin())
				{
					ViewState->TabIndices.Add(StateKey, NewIndex);
				}
			})
			.Tabs(Tabs)
			.Panes(Panes)
			.TabBarExtra(TabBarExtra);
	}

	if (AddButton.IsValid())
	{
		return SNew(SBox)
			.HAlign(HAlign_Right)
			.Padding(4, 2)
			[
				AddButton.ToSharedRef()
			];
	}

	return nullptr;
}

void FEpithWindowElement_TabbedGroup::PostSerialize(const FArchive& Ar)
{
	if (Ar.IsLoading() && bBorder_DEPRECATED)
	{
		Border = 1;
		bBorder_DEPRECATED = false;
	}
}

#undef LOCTEXT_NAMESPACE
