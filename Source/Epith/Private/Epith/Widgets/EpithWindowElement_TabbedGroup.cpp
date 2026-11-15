#include "Epith/Widgets/EpithWindowElement_TabbedGroup.h"

#include "Epith/Widgets/EpithDrawContext.h"
#include "Epith/Widgets/SEpithTabbedPane.h"

#define LOCTEXT_NAMESPACE "Epith"

TSharedPtr<SWidget> FEpithWindowElement_TabbedGroup::Draw(const FEpithDrawContext& Context) const
{
	// In the designer, slots are edited in place (see EpithDraw::DrawSlot)
	TArray<FEpithWindowElement_TabbedGroup_Slot>& MutableSlots = const_cast<TArray<FEpithWindowElement_TabbedGroup_Slot>&>(Slots);
	
	TArray<FText> Tabs;
	TArray<TSharedRef<SWidget>> Panes;
	
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		FText TabText = FText::FromString(Slots[i].TabName);
		
		if (TabText.IsEmpty())
		{
			TabText = FText::Format(LOCTEXT("TabbedGroup_DefaultTabName", "Tab {0}"), i + 1);
		}
		
		const FEpithSlotActions Actions = Context.IsDesigning() ? EpithDraw::MakeArraySlotActions(MutableSlots, i) : FEpithSlotActions();
		
		TSharedPtr<SWidget> PaneWidget = EpithDraw::DrawSlot(Slots[i].Element, Context, Actions);
		
		if (!PaneWidget.IsValid())
		{
			continue;
		}
		
		Tabs.Add(TabText);
		Panes.Add(PaneWidget.ToSharedRef());
	}
	
	TSharedPtr<SWidget> TabbedPane;
	
	if (Panes.Num() > 0)
	{
		TabbedPane = SNew(SEpithTabbedPane)
			.Tabs(Tabs)
			.Panes(Panes);
	}
	
	if (!Context.IsDesigning())
	{
		return TabbedPane;
	}
	
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	
	if (TabbedPane.IsValid())
	{
		Box->AddSlot()
		.AutoHeight()
		[
			TabbedPane.ToSharedRef()
		];
	}
	
	Box->AddSlot()
	.AutoHeight()
	.Padding(4, 2)
	[
		Context.Designer->MakeAddButton(LOCTEXT("TabbedGroup_AddTab", "Add Tab"), [SlotsPtr = &MutableSlots]()
		{
			FEpithWindowElement_TabbedGroup_Slot& NewSlot = SlotsPtr->AddDefaulted_GetRef();
			NewSlot.TabName = FString::Printf(TEXT("Tab %d"), SlotsPtr->Num());
		})
	];
	
	return Box;
}

#undef LOCTEXT_NAMESPACE
