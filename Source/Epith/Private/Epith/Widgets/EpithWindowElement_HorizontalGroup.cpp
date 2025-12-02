#include "Epith/Widgets/EpithWindowElement_HorizontalGroup.h"

#include "Epith/Style/EpithColor.h"
#include "Epith/Style/EpithStyle.h"

TSharedPtr<SWidget> FEpithWindowElement_HorizontalGroup::Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const
{
	TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox);
	
	int32 SlotsNum = Slots.Num();
	
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		const FEpithWindowElement_HorizontalGroup_Slot& Slot = Slots[i];
		
		const TInstancedStruct<FEpithWindowElement>& Child = Slot.Child;
		
		if (!Child.IsValid())
		{
			continue;
		}
		
		TSharedPtr<SWidget> Widget = Child.Get<FEpithWindowElement>().Draw(InPropertyData);
		
		if (!Widget.IsValid())
		{
			continue;
		}
		
		Box->AddSlot()
		.AutoWidth()
		.Padding(4, 4, i == SlotsNum - 1 ? 4 : 0, 4)
		[
			SNew(SBorder)
			.Padding(Slot.DisplayParams.Padding)
			.BorderImage(Slot.DisplayParams.bBorder ? FEpithStyle::GetImageBrush(EpithBrushes.Border_2px_Deburred) : FAppStyle::GetBrush("NoBorder"))
			.BorderBackgroundColor(EpithColor::Noir)
			[
				Widget.ToSharedRef()
			]
		];
	}
	
	return Box;
}
