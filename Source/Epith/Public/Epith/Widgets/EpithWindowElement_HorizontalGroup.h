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

	TSharedPtr<SWidget> Draw(const FEpithDrawContext& Context) const override;

	/** Border around the whole group: 0 = none, 1 = thin, 2 = thick. (Each slot can also have its own border.) */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = 0, ClampMax = 2, UIMin = 0, UIMax = 2))
	uint8 Border = 0;

	/** Column widths for the property rows in this group. */
	UPROPERTY(EditDefaultsOnly)
	FEpithColumnSettings Columns;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Horizontal Group Slots", meta = (ExcludeBaseStruct))
	TArray<FEpithWindowElement_HorizontalGroup_Slot> Slots;
};
