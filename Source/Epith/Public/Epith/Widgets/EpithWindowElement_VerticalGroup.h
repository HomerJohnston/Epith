#pragma once

#include "EpithWindowElement.h"

#include "EpithWindowElement_VerticalGroup.generated.h"

USTRUCT()
struct FEpithWindowElement_VerticalGroup_Slot
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, DisplayName = "Slot Appearance")
	FEpithWindowElement_BoxDisplayParams DisplayParams;
	
	UPROPERTY(EditDefaultsOnly, DisplayName = "Element", meta = (ShowOnlyInnerProperties, ExcludeBaseStruct))
	TInstancedStruct<FEpithWindowElement> Child;
};

USTRUCT(DisplayName = "Vertical Group")
struct FEpithWindowElement_VerticalGroup : public FEpithWindowElement
{
	GENERATED_BODY()

	virtual ~FEpithWindowElement_VerticalGroup() {}
	
	TSharedPtr<SWidget> Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const override;
	
	UPROPERTY(EditDefaultsOnly, DisplayName = "Vertical Group Slots", meta = (ExcludeBaseStruct))
	TArray<FEpithWindowElement_VerticalGroup_Slot> Slots;
};
