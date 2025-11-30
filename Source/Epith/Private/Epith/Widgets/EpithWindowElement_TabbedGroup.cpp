#include "Epith/Widgets/EpithWindowElement_TabbedGroup.h"

#include "Epith/Widgets/SEpithTabbedPane.h"

TSharedPtr<SWidget> FEpithWindowElement_TabbedGroup::Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const
{
	int32 SlotsNum = Slots.Num();
	
	TArray<FText> Tabs;
	TArray<TSharedRef<SWidget>> Panes;
	
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		Tabs.Add(INVTEXT("Tab"));
		Panes.Add(Slots[i].Element.Get<FEpithWindowElement>().Draw(InPropertyData).ToSharedRef());
	}
	
	return SNew(SEpithTabbedPane)
		.Tabs(Tabs)
		.Panes(Panes);
}
