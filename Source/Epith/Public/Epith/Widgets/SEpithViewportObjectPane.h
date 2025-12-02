#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"

struct FEpithWindowElement;

class SEpithViewportObjectPane : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SEpithViewportObjectPane) {}

		SLATE_ARGUMENT(UObject*, Target)

		SLATE_ARGUMENT(const FEpithWindowElement*, Root)
		
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	
protected:
	const FEpithWindowElement* Root;
	TWeakObjectPtr<UObject> Target;
	TSharedPtr<FEpithPropertyDataContainer> PropertyData;
	
protected:
	void OnObjectsReinstanced(const TMap<UObject*, UObject*>& ReplacementMap);
	
	void Build();
};
