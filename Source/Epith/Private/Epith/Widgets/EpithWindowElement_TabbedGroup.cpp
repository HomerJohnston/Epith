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
		
		Tabs.Add(TabText);
		Panes.Add(Slots[i].Element.Get<FEpithWindowElement>().Draw(InPropertyData).ToSharedRef());
	}
	
	return SNew(SEpithTabbedPane)
		.Tabs(Tabs)
		.Panes(Panes);
}
