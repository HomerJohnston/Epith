#include "Epith/Widgets/SEpithActorPane.h"

#include "Epith/Widgets/EpithWindowElement.h"

void SEpithActorPane::Construct(const FArguments& InArgs)
{
	PropertyData = InArgs._PropertyData;
	
	if (!InArgs._Root)
	{
		return;
	}
	
	ChildSlot
	[
		InArgs._Root->Draw(PropertyData).ToSharedRef()
	];
}
