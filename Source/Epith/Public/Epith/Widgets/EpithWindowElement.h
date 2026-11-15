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
	uint8 Padding = 4;
};

USTRUCT(meta = (Hidden))
struct FEpithWindowElement
{
	GENERATED_BODY()

	/** Builds this element's widget. Use EpithDraw::DrawSlot (EpithDrawContext.h) to draw child slots. */
	virtual TSharedPtr<SWidget> Draw(const FEpithDrawContext& Context) const { return nullptr; }
};
