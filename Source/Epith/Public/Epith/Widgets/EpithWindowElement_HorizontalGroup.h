#pragma once

#include "EpithWindowElement.h"

#include "EpithWindowElement_HorizontalGroup.generated.h"

USTRUCT()
struct FEpithWindowElement_HorizontalGroup_Slot
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, DisplayName = "Slot Appearance")
	FEpithWindowElement_BoxDisplayParams DisplayParams;
	
	UPROPERTY(EditDefaultsOnly, DisplayName = "Element", meta = (ShowOnlyInnerProperties, ExcludeBaseStruct))
	TInstancedStruct<FEpithWindowElement> Child;
};

USTRUCT(DisplayName = "Horizontal Group")
struct FEpithWindowElement_HorizontalGroup : public FEpithWindowElement
{
	GENERATED_BODY()
	
	virtual ~FEpithWindowElement_HorizontalGroup() {}
	
	TSharedPtr<SWidget> Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const override;
	
	UPROPERTY(EditDefaultsOnly, DisplayName = "Horizontal Group Slots", meta = (ExcludeBaseStruct))
	TArray<FEpithWindowElement_HorizontalGroup_Slot> Slots;
};
