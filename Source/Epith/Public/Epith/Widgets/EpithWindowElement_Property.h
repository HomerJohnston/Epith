#pragma once

#include "EpithWindowElement.h"

#include "EpithWindowElement_Property.generated.h"

USTRUCT(DisplayName = "Property")
struct FEpithWindowElement_Property : public FEpithWindowElement
{
	GENERATED_BODY()
	
	/** If unspecified, uses the actor name (default category) */
	UPROPERTY(EditDefaultsOnly)
	FName Category;
	
	/** Property name on the actor */
	UPROPERTY(EditDefaultsOnly)
	FName Property;
	
	TSharedPtr<SWidget> Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const override;
};