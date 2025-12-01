#include "Epith/Widgets/SEpithTabbedPane.h"

#include "Epith/EpithLog.h"
#include "Epith/Style/EpithColor.h"
#include "Epith/Style/EpithStyle.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

void SEpithTabbedPane::Construct(const FArguments& InArgs)
{
	uint8 Count = InArgs._Tabs.Num();
	
	if (InArgs._Panes.Num() != Count)
	{
		UE_LOG(LogEpith, Error, TEXT("Tried to spawn SEpithTabbedPane with unmatching number of Tabs and Panes"));
		return;
	}
	
	TSharedRef<SSegmentedControl<uint8>> TabBox = SNew(SSegmentedControl<uint8>)
		.Style(FEpithStyle::Get(), EpithStyles.SegmentedControlStyle_Tab)
		.Value(this, &SEpithTabbedPane::Value_Tab)
		.OnValueChanged(this, &SEpithTabbedPane::OnValueChanged_Tab);
	
	TSharedRef<SWidgetSwitcher> Panes = SNew(SWidgetSwitcher)
		.WidgetIndex(this, &SEpithTabbedPane::WidgetIndex_TabPane);
	
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			TabBox
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FEpithStyle::GetImageBrush(EpithBrushes.Background_WhiteSquare))
			.BorderBackgroundColor(EpithColor::DeepGray)
			//.ColorAndOpacity(EpithColor::DarkGray)
			.Padding(4, 4, 4, 4)
			[
				Panes
			]
		]
	];

	for (uint8 i = 0; i < Count; ++i)
	{
		TabBox->AddSlot(i)
		.Text(InArgs._Tabs[i]);
		
		Panes->AddSlot(i)
		[
			InArgs._Panes[i]
		];
	}
}

uint8 SEpithTabbedPane::Value_Tab() const
{
	return CurrentIndex;
}

void SEpithTabbedPane::OnValueChanged_Tab(uint8 NewIndex)
{
	CurrentIndex = NewIndex;
}

int SEpithTabbedPane::WidgetIndex_TabPane() const
{
	return CurrentIndex;
}
