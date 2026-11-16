#pragma once

class SEpithTabbedPane : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SEpithTabbedPane)
		: _InitialIndex(0)
		, _Border(0)
		{}
		/** The tab to show first (e.g. the one that was showing before a redraw). */
		SLATE_ARGUMENT(uint8, InitialIndex)
		/** Called when the user switches tabs. */
		SLATE_ARGUMENT(TFunction<void(uint8)>, OnTabChanged)
		/** Border below the tab bar, around the panes: 0 = none, 1 = thin, 2 = thick. */
		SLATE_ARGUMENT(uint8, Border)
		/** Shown to the right of the tabs (the designer's add button). */
		SLATE_ARGUMENT(TSharedPtr<SWidget>, TabBarExtra)

		SLATE_ARGUMENT(TArray<FText>, Tabs)

		SLATE_ARGUMENT(TArray<TSharedRef<SWidget>>, Panes)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	uint8 CurrentIndex = 0;
	TFunction<void(uint8)> OnTabChanged;

	uint8 Value_Tab() const;

	void OnValueChanged_Tab(uint8 NewIndex);

	int WidgetIndex_TabPane() const;

};