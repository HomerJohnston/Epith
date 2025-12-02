#include "Epith/Widgets/SEpithViewportObjectPane.h"

#include "Epith/Style/EpithColor.h"
#include "Epith/Style/EpithStyle.h"
#include "Epith/Widgets/EpithWindowElement.h"
#include "Epith/Widgets/SEpithViewportObjectPaneHeader.h"
#include "Styling/SlateIconFinder.h"

void SEpithViewportObjectPane::Construct(const FArguments& InArgs)
{
	PropertyData = InArgs._PropertyData;
	
	if (!InArgs._Root || !InArgs._Target)
	{
		return;
	}
			
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage( FAppStyle::GetBrush("Menu.Background") )
		.BorderBackgroundColor(EpithColor::White_BarelyTrans)
		.Padding(8)
		[
			SNew(SBox)
			.MinDesiredWidth(180)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(12, 12, 12, 8)
				[
					SNew(SEpithViewportObjectPaneHeader)
					.Target(InArgs._Target)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					InArgs._Root->Draw(PropertyData).ToSharedRef()
				]
			]
		]
	];
}
