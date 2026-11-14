#pragma once

#include "EpithPropertySelector.generated.h"

/**
 * Names a property on the panel definition's TargetClass by its property path (as produced by IPropertyHandle::GeneratePathToProperty).
 * Edited through FPropertyTypeCustomization_EpithPropertySelector, which offers a dropdown of valid paths.
 */
USTRUCT()
struct FEpithPropertySelector
{
	GENERATED_BODY()

	friend class FPropertyTypeCustomization_EpithPropertySelector;
	
protected:
	UPROPERTY(EditAnywhere)
	FName PropertyPath;

public:
	FName GetPropertyPath() const;
};