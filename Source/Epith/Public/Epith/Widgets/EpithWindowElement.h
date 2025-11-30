#pragma once

#include "StructUtils/InstancedStruct.h"

#include "EpithWindowElement.generated.h"

struct FEpithPropertyDataContainer;

USTRUCT(meta = (Hidden))
struct FEpithWindowElement
{
	GENERATED_BODY()

	virtual TSharedPtr<SWidget> Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const { return nullptr; }
};
