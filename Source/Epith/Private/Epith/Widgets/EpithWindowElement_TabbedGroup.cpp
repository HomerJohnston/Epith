#include "Epith/Widgets/EpithWindowElement_TabbedGroup.h"

#include "Epith/Widgets/SEpithTabbedPane.h"

TSharedPtr<SWidget> FEpithWindowElement_TabbedGroup::Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const
{
	int32 SlotsNum = Slots.Num();
	
	TArray<FText> Tabs;
	TArray<TSharedRef<SWidget>> Panes;
	
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (!Slots[i].Element.IsValid())
		{
			continue;
		}
		
		FText TabText = FText::FromString(Slots[i].TabName);
		
		if (TabText.IsEmpty())
		{
			TabText = FText::Format(INVTEXT("Tab {0}"), i);
		}
		
		TSharedPtr<SWidget> PaneWidget = Slots[i].Element.Get<FEpithWindowElement>().Draw(InPropertyData);
		
		if (!PaneWidget.IsValid())
		{
			continue;
		}
		
		Tabs.Add(TabText);
		Panes.Add(PaneWidget.ToSharedRef());
	}
	
	if (Panes.Num() == 0)
	{
		return nullptr;
	}
	
	return SNew(SEpithTabbedPane)
		.Tabs(Tabs)
		.Panes(Panes);
}
