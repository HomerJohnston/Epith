#pragma once

#include "EpithSpecialControl.h"
#include "Components/ActorComponent.h"
#include "Templates/SubclassOf.h"

#include "EpithSpecialControl_RemoveComponent.generated.h"

/**
 * A button that removes the actor's component of the chosen class, the same way deleting it in the Details panel does (undoable).
 * Only components added to this actor in the level can be removed; components that are part of the actor's class can't.
 */
USTRUCT(DisplayName = "Remove Component Button")
struct EPITH_API FEpithSpecialControl_RemoveComponent : public FEpithSpecialControl
{
	GENERATED_BODY()
	
	/** The class of component to remove. */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UActorComponent> ComponentClass;
	
	/** The button's text. Empty = "Remove <component class>". */
	UPROPERTY(EditDefaultsOnly)
	FText ButtonText;
	
	/** Remove every removable component of this class. Off = remove only the first one. */
	UPROPERTY(EditDefaultsOnly)
	bool bRemoveAll = false;
	
	TSharedPtr<SWidget> Draw(const FEpithControlContext& Context) const override;
	
	/** The actor's components of ComponentClass that can be removed (ones added in the level, not part of the actor's class). */
	static TArray<UActorComponent*> FindRemovableComponents(AActor* Actor, TSubclassOf<UActorComponent> ComponentClass);
	
	/** Removes the given components from the actor, as one undoable change. Returns how many were removed (with a reason if not all). */
	static int32 RemoveComponents(AActor* Actor, const TArray<UActorComponent*>& Components, FText& OutFailReason);
};
