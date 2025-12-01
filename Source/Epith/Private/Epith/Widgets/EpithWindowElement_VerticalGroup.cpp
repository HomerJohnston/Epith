#include "Epith/Widgets/EpithWindowElement_VerticalGroup.h"

#include "Epith/Style/EpithColor.h"
#include "Epith/Style/EpithStyle.h"

TSharedPtr<SWidget> FEpithWindowElement_VerticalGroup::Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	
	int32 SlotsNum = Slots.Num();
	
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		const FEpithWindowElement_VerticalGroup_Slot& Slot = Slots[i];
		
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
		.AutoHeight()
		.Padding(4, 4, 4, i == SlotsNum - 1 ? 4 : 0)
		[
			SNew(SBorder)
			.Padding(Slot.DisplayParams.Padding)
			.BorderImage(Slot.DisplayParams.bHideBorder ? FAppStyle::GetBrush("NoBorder") : FEpithStyle::GetImageBrush(EpithBrushes.Border_2px_Deburred))
			.BorderBackgroundColor(EpithColor::Noir)
			[
				Widget.ToSharedRef()
			]
		];
	}
	
	return Box;
}
