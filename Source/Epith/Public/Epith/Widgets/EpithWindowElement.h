#pragma once

#include "StructUtils/InstancedStruct.h"

#include "EpithWindowElement.generated.h"

struct FEpithDrawContext;

USTRUCT()
struct FEpithWindowElement_BoxDisplayParams
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	bool bBorder = false;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = 0, ClampMax = 40, UIMin = 0, UIMax = 20))
	uint8 Padding = 0;
};

/** Column widths for the property rows directly inside a group (and their nested struct/array rows), so names and values line up. */
USTRUCT()
struct FEpithColumnSettings
{
	GENERATED_BODY()
	
	/** Width of the name column (including indentation for nested rows). 0 = as wide as the widest name in the group. */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = 0, UIMin = 0, UIMax = 600))
	float NameColumnWidth = 0.0f;
	
	/** Width of the value column. 0 = each value takes the width it needs. */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = 0, UIMin = 0, UIMax = 600))
	float ValueColumnWidth = 0.0f;
};

USTRUCT(meta = (Hidden))
struct FEpithWindowElement
{
	GENERATED_BODY()

	/** Builds this element's widget. Use EpithDraw::DrawSlot (EpithDrawContext.h) to draw child slots. */
	virtual TSharedPtr<SWidget> Draw(const FEpithDrawContext& Context) const { return nullptr; }
};
