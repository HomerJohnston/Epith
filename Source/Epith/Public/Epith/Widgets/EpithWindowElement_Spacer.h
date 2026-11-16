#pragma once

#include "EpithWindowElement.h"

#include "EpithWindowElement_Spacer.generated.h"

/** An 8 px gap: tall in a Vertical Group, wide in a Horizontal Group. */
USTRUCT(DisplayName = "Spacer")
struct FEpithWindowElement_Spacer : public FEpithWindowElement
{
	GENERATED_BODY()
	
	virtual ~FEpithWindowElement_Spacer() {}
	
	static constexpr float Size = 8.0f;
	
	/** Fill the gap with the editor's dark background color, instead of leaving it see-through. */
	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	bool bFilled = false;
	
	TSharedPtr<SWidget> Draw(const FEpithDrawContext& Context) const override;
};
