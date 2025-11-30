#pragma once

#include "EpithWindowElement.h"

#include "EpithWindowElement_HorizontalGroup.generated.h"

USTRUCT()
struct FEpithWindowElement_HorizontalGroup_Slot
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	bool bHideBorder = false;
	
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = 0, ClampMax = 40, UIMin = 0, UIMax = 20))
	uint8 Padding = 8;
	
	UPROPERTY(EditDefaultsOnly, meta = (ShowOnlyInnerProperties, ExcludeBaseStruct))
	TInstancedStruct<FEpithWindowElement> Child;
};

USTRUCT(DisplayName = "Horizontal Group")
struct FEpithWindowElement_HorizontalGroup : public FEpithWindowElement
{
	GENERATED_BODY()
	
	virtual ~FEpithWindowElement_HorizontalGroup() {}
	
	TSharedPtr<SWidget> Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const override;
	
	UPROPERTY(EditDefaultsOnly, meta = (ExcludeBaseStruct))
	TArray<FEpithWindowElement_HorizontalGroup_Slot> Slots;
};
