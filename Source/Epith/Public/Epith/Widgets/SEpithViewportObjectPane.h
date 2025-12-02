#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"

struct FEpithWindowElement;

class SEpithViewportObjectPane : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SEpithViewportObjectPane) {}

		SLATE_ARGUMENT(UObject*, Target)
		
		SLATE_ARGUMENT(TSharedPtr<FEpithPropertyDataContainer>, PropertyData)

		SLATE_ARGUMENT(const FEpithWindowElement*, Root)
		
	SLATE_END_ARGS()
	
	TSharedPtr<FEpithPropertyDataContainer> PropertyData;
	
	void Construct(const FArguments& InArgs);
};
