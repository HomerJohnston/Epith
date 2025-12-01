#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"

struct FEpithWindowElement;

class SEpithActorPane : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SEpithActorPane) {}

		SLATE_ARGUMENT(FText, Title)
		
		SLATE_ARGUMENT(TSharedPtr<FEpithPropertyDataContainer>, PropertyData)

		SLATE_ARGUMENT(const FEpithWindowElement*, Root)
		
	SLATE_END_ARGS()
	
	TSharedPtr<FEpithPropertyDataContainer> PropertyData;
	
	void Construct(const FArguments& InArgs);
};
