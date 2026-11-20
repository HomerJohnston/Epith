#pragma once

#include "EpithWindowElement.h"

#include "EpithWindowElement_Divider.generated.h"

/**
 * An 8 px gap (plus 4 px either side) between the slots of a Vertical or Horizontal Group, optionally filled as a bar. Only
 * allowed in those two groups. (Called "Spacer" until it was renamed; old assets are redirected, see Config/DefaultEpith.ini.)
 */
USTRUCT(DisplayName = "Divider")
struct FEpithWindowElement_Divider : public FEpithWindowElement
{
	GENERATED_BODY()
	
	virtual ~FEpithWindowElement_Divider() {}
	
	static constexpr float Size = 8.0f;
	
	/** Space on both sides of the gap along the group's direction: above and below in a Vertical Group, left and right in a Horizontal Group. */
	static constexpr float PaddingAlongGroup = 4.0f;
	
	/**
	 * Fill the gap with the editor's dark background color, instead of leaving it see-through.
	 * Filled dividers join up with filled dividers in a neighbouring group of the other direction, making T and + shapes:
	 * a Horizontal Group's bar reaches up and down to a bar beside it in the Vertical Group around it, and the other way round.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	bool bFilled = false;
	
	TSharedPtr<SWidget> Draw(const FEpithDrawContext& Context) const override;
};
