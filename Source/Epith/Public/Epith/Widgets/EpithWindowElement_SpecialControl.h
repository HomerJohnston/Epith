#pragma once

#include "EpithWindowElement.h"
#include "Epith/Controls/EpithSpecialControl.h"

#include "EpithWindowElement_SpecialControl.generated.h"

/** A leaf element that shows a special control (see FEpithSpecialControl), such as a button that runs code on the actor. */
USTRUCT(DisplayName = "Special Control")
struct FEpithWindowElement_SpecialControl : public FEpithWindowElement
{
	GENERATED_BODY()
	
	virtual ~FEpithWindowElement_SpecialControl() {}
	
	/** The control to show, and its settings. */
	UPROPERTY(EditDefaultsOnly, meta = (ExcludeBaseStruct))
	TInstancedStruct<FEpithSpecialControl> Control;
	
	TSharedPtr<SWidget> Draw(const FEpithDrawContext& Context) const override;
};
