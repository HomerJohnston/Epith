#pragma once

#include "EpithWindowElement.h"

#include "EpithWindowElement_VerticalGroup.generated.h"

USTRUCT()
struct FEpithWindowElement_VerticalGroup_Slot
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	bool bHideBorder = false;
	
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = 0, ClampMax = 24, UIMin = 0, UIMax = 24))
	uint8 Padding = 8;
	
	UPROPERTY(EditDefaultsOnly, meta = (ShowOnlyInnerProperties, ExcludeBaseStruct))
	TInstancedStruct<FEpithWindowElement> Child;
};

USTRUCT(DisplayName = "Vertical Group")
struct FEpithWindowElement_VerticalGroup : public FEpithWindowElement
{
	GENERATED_BODY()

	virtual ~FEpithWindowElement_VerticalGroup() {}
	
	TSharedPtr<SWidget> Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const override;
	
	UPROPERTY(EditDefaultsOnly, meta = (ExcludeBaseStruct))
	TArray<FEpithWindowElement_VerticalGroup_Slot> Slots;
};
