#pragma once

class SEpithViewportObjectPane;

class SEpithViewportObjectPaneHeader : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SEpithViewportObjectPaneHeader) {}

		SLATE_ARGUMENT(UObject*, Target)
		
		/** If not empty, shown instead of the automatic title. */
		SLATE_ARGUMENT(FText, TitleOverride)

	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
};
