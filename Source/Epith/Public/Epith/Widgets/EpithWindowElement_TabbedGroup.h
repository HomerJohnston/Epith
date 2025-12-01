#pragma once

#include "EpithWindowElement.h"

#include "EpithWindowElement_TabbedGroup.generated.h"

USTRUCT()
struct FEpithWindowElement_TabbedGroup_SlotDisplayParams
{
	GENERATED_BODY()
};

USTRUCT()
struct FEpithWindowElement_TabbedGroup_Slot
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	FString TabName;
	
	//UPROPERTY(EditDefaultsOnly, DisplayName = "Slot Appearance")
	//FEpithWindowElement_TabbedGroup_SlotDisplayParams DisplayParams;
	
	UPROPERTY(EditDefaultsOnly, meta = (ShowOnlyInnerProperties, ExcludeBaseStruct))
	TInstancedStruct<FEpithWindowElement> Element;
};

USTRUCT(DisplayName = "Tabbed Group")
struct FEpithWindowElement_TabbedGroup : public FEpithWindowElement
{
	GENERATED_BODY()
	
	virtual ~FEpithWindowElement_TabbedGroup() {}
	
	TSharedPtr<SWidget> Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const override;
	
	UPROPERTY(EditDefaultsOnly, DisplayName = "Tabbed Group Slots",meta = (ExcludeBaseStruct))
	TArray<FEpithWindowElement_TabbedGroup_Slot> Slots;
};
