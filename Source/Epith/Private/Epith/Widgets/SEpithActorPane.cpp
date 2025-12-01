#include "Epith/Widgets/SEpithActorPane.h"

#include "Epith/Style/EpithStyle.h"
#include "Epith/Widgets/EpithWindowElement.h"

void SEpithActorPane::Construct(const FArguments& InArgs)
{
	PropertyData = InArgs._PropertyData;
	
	if (!InArgs._Root)
	{
		return;
	}
	
	FText Title = InArgs._Title;
	
	if (Title.IsEmpty())
	{
		Title = INVTEXT("Actor Properties");
	};
	
	ChildSlot
	[
		SNew(SBox)
		.MinDesiredWidth(180)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(12, 12, 12, 8)
			[
				SNew(STextBlock)
				.Text(Title)
				.Font(EpithFonts.Font_ViewportPopup_Title)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				InArgs._Root->Draw(PropertyData).ToSharedRef()
			]
		]
	];
}
