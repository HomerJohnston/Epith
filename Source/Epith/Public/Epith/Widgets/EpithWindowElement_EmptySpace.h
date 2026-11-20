#pragma once

#include "EpithWindowElement.h"

#include "EpithWindowElement_EmptySpace.generated.h"

/**
 * Nothing at all: a blank element that just fills its slot, e.g. to leave part of a Horizontal Group's width empty (it takes
 * its slot's share like any other element). The panel designer shows it as a faint placeholder so it can be picked.
 *
 * Shown as "Spacer". (Its C++ name differs because the old Spacer element, now the Divider, keeps redirecting from that name.)
 */
USTRUCT(DisplayName = "Spacer")
struct FEpithWindowElement_EmptySpace : public FEpithWindowElement
{
	GENERATED_BODY()

	virtual ~FEpithWindowElement_EmptySpace() {}

	TSharedPtr<SWidget> Draw(const FEpithDrawContext& Context) const override;
};
