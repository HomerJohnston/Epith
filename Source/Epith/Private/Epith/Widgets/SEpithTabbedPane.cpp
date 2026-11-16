#include "Epith/Widgets/SEpithTabbedPane.h"

#include "Epith/EpithLog.h"
#include "Epith/Style/EpithColor.h"
#include "Epith/Style/EpithStyle.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

void SEpithTabbedPane::Construct(const FArguments& InArgs)
{
	uint8 Count = InArgs._Tabs.Num();

	if (InArgs._Panes.Num() != Count)
	{
		UE_LOG(LogEpith, Error, TEXT("Tried to spawn SEpithTabbedPane with unmatching number of Tabs and Panes"));
		return;
	}

	CurrentIndex = Count > 0 ? FMath::Min<uint8>(InArgs._InitialIndex, Count - 1) : 0;
	OnTabChanged = InArgs._OnTabChanged;

	TSharedRef<SSegmentedControl<uint8>> TabBox = SNew(SSegmentedControl<uint8>)
		.Style(FEpithStyle::Get(), EpithStyles.SegmentedControlStyle_Tab)
		.Value(this, &SEpithTabbedPane::Value_Tab)
		.OnValueChanged(this, &SEpithTabbedPane::OnValueChanged_Tab);

	TSharedRef<SWidgetSwitcher> Panes = SNew(SWidgetSwitcher)
		.WidgetIndex(this, &SEpithTabbedPane::WidgetIndex_TabPane);

	TSharedRef<SWidget> PaneArea = SNew(SBorder)
		.BorderImage(FEpithStyle::GetImageBrush(EpithBrushes.Background_WhiteSquare))
		.BorderBackgroundColor(EpithColor::DeepGray_BarelyTrans)
		.Padding(4, 4, 4, 4)
		[
			Panes
		];

	// A U-shaped border around everything below the tab bar: the tabs close it at the top
	if (InArgs._Border > 0)
	{
		PaneArea = SNew(SBorder)
			.BorderImage(FEpithStyle::GetBorderBrush(InArgs._Border, true))
			.BorderBackgroundColor(EpithColor::Noir_SemiTrans)
			.Padding(FEpithStyle::GetBorderPadding(InArgs._Border, true))
			[
				PaneArea
			];
	}

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				TabBox
			]
			// No padding here, so without an extra widget the tabs reach the right edge; the extra widget brings its own spacing
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				InArgs._TabBarExtra.IsValid() ? InArgs._TabBarExtra.ToSharedRef() : SNullWidget::NullWidget
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			PaneArea
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

	if (OnTabChanged)
	{
		OnTabChanged(NewIndex);
	}
}

int SEpithTabbedPane::WidgetIndex_TabPane() const
{
	return CurrentIndex;
}
