#pragma once

#include "IEpithActorInterface.generated.h"

UINTERFACE(MinimalAPI)
class UEpithActorInterface : public UInterface
{
	GENERATED_BODY()
};

class EPITH_API IEpithActorInterface
{
	GENERATED_BODY()
	
public:
	virtual void EpithBootstrapFunction() {};
};