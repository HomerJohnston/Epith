#pragma once

#include "StructUtils/InstancedStruct.h"

#include "EpithWindowElement.generated.h"

struct FEpithDrawContext;

USTRUCT()
struct FEpithWindowElement_BoxDisplayParams
{
	GENERATED_BODY()

	/** Border: 0 = none, 1 = thin, 2 = thick. */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = 0, ClampMax = 2, UIMin = 0, UIMax = 2))
	uint8 Border = 0;

	/** The old on/off border setting; old assets load it and PostSerialize turns it into Border = thin. */
	UPROPERTY()
	bool bBorder_DEPRECATED = false;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = 0, ClampMax = 40, UIMin = 0, UIMax = 20))
	uint8 Padding = 0;

	/** Moves the old bBorder setting into Border. */
	void PostSerialize(const FArchive& Ar);
};

template<>
struct TStructOpsTypeTraits<FEpithWindowElement_BoxDisplayParams> : public TStructOpsTypeTraitsBase2<FEpithWindowElement_BoxDisplayParams>
{
	enum
	{
		WithPostSerialize = true,
	};
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
