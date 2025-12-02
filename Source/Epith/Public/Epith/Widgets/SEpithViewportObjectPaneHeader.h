#pragma once

class SEpithViewportObjectPane;

class SEpithViewportObjectPaneHeader : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SEpithViewportObjectPaneHeader) {}

		SLATE_ARGUMENT(UObject*, Target)
		
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
};
