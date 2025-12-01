#pragma once

#include "EpithPropertySelector.generated.h"

/** 
 *  
 */
USTRUCT()
struct FEpithPropertySelector
{
	GENERATED_BODY()

	friend class FPropertyTypeCustomization_EpithPropertySelector;
	
protected:
	UPROPERTY(EditAnywhere)
	UObject* Object;
	
	UPROPERTY(EditAnywhere, meta = (GetOptions = "GetPropertyPaths"))
	FName PropertyPath;

public:
	FName GetPropertyPath() const;
};