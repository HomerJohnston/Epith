#pragma once

#include "StructUtils/InstancedStruct.h"

#include "EpithWindowElement.generated.h"

struct FEpithPropertyDataContainer;

USTRUCT()
struct FEpithWindowElement_BoxDisplayParams
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	bool bBorder = false;
	
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = 0, ClampMax = 40, UIMin = 0, UIMax = 20))
	uint8 Padding = 8;
};

USTRUCT(meta = (Hidden))
struct FEpithWindowElement
{
	GENERATED_BODY()

	virtual TSharedPtr<SWidget> Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const { return nullptr; }
};
