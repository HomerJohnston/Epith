#pragma once

#include "EpithWindowElement.h"

#include "EpithWindowElement_Spacer.generated.h"

/** An 8 px gap (plus 4 px either side) between the slots of a Vertical or Horizontal Group. Only allowed in those two groups. */
USTRUCT(DisplayName = "Spacer")
struct FEpithWindowElement_Spacer : public FEpithWindowElement
{
	GENERATED_BODY()
	
	virtual ~FEpithWindowElement_Spacer() {}
	
	static constexpr float Size = 8.0f;
	
	/** Space on both sides of the gap along the group's direction: above and below in a Vertical Group, left and right in a Horizontal Group. */
	static constexpr float PaddingAlongGroup = 4.0f;
	
	/** Fill the gap with the editor's dark background color, instead of leaving it see-through. */
	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	bool bFilled = false;
	
	TSharedPtr<SWidget> Draw(const FEpithDrawContext& Context) const override;
};
