#pragma once

class SEpithTabbedPane : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SEpithTabbedPane)
		{}
		
		SLATE_ARGUMENT(TArray<FText>, Tabs)

		SLATE_ARGUMENT(TArray<TSharedRef<SWidget>>, Panes)
		
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	uint8 CurrentIndex = 0;
	
	uint8 Value_Tab() const;
	
	void OnValueChanged_Tab(uint8 NewIndex);
	
	int WidgetIndex_TabPane() const;
	
};