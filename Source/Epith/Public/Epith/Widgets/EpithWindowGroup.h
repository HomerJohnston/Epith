#pragma once

#include "EpithWindowElement.h"

class FEpithWindowGroup : public FEpithWindowElement
{
	virtual TSharedRef<SWidget> Draw() = 0;
};
