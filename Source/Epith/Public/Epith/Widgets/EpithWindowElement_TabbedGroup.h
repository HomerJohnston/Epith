#pragma once

#include "EpithWindowElement.h"

#include "EpithWindowElement_TabbedGroup.generated.h"

USTRUCT()
struct FEpithWindowElement_TabbedGroup_Slot
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, meta = (ShowOnlyInnerProperties, ExcludeBaseStruct))
	TInstancedStruct<FEpithWindowElement> Element;
};

USTRUCT(DisplayName = "Tabbed Group")
struct FEpithWindowElement_TabbedGroup : public FEpithWindowElement
{
	GENERATED_BODY()
	
	virtual ~FEpithWindowElement_TabbedGroup() {}
	
	TSharedPtr<SWidget> Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const override;
	
	UPROPERTY(EditDefaultsOnly, meta = (ExcludeBaseStruct))
	TArray<FEpithWindowElement_TabbedGroup_Slot> Slots;
};
