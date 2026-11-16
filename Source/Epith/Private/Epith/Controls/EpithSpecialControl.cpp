#include "Epith/Controls/EpithSpecialControl.h"

#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"

FEpithControlContext::FEpithControlContext(UObject* InTarget, bool bInPreview)
	: Target(InTarget)
	, bPreview(bInPreview)
{
}

UObject* FEpithControlContext::GetTarget() const
{
	return Target.Get();
}

AActor* FEpithControlContext::GetActor() const
{
	UObject* Object = Target.Get();
	
	if (AActor* Actor = Cast<AActor>(Object))
	{
		return Actor;
	}
	
	if (UActorComponent* Component = Cast<UActorComponent>(Object))
	{
		return Component->GetOwner();
	}
	
	return Object ? Object->GetTypedOuter<AActor>() : nullptr;
}

bool FEpithControlContext::IsPreview() const
{
	return bPreview;
}
