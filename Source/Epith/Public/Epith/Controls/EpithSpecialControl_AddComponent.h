#pragma once

#include "EpithSpecialControl.h"
#include "Components/ActorComponent.h"
#include "Templates/SubclassOf.h"

#include "EpithSpecialControl_AddComponent.generated.h"

/** A button that adds a component of the chosen class to the actor, the same way the Details panel's "Add" button does (undoable). */
USTRUCT(DisplayName = "Add Component Button")
struct EPITH_API FEpithSpecialControl_AddComponent : public FEpithSpecialControl
{
	GENERATED_BODY()

	/** The class of component to add. */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UActorComponent> ComponentClass;

	/** The button's text. Empty = "Add <component class>". */
	UPROPERTY(EditDefaultsOnly)
	FText ButtonText;

	/** Allow adding another one when the actor already has a component of this class. Off = the button is disabled then. */
	UPROPERTY(EditDefaultsOnly)
	bool bAllowDuplicates = false;

	TSharedPtr<SWidget> Draw(const FEpithControlContext& Context) const override;

	/**
	 * Adds a component of ComponentClass to the actor, as one undoable change, and returns it so the caller can set it up.
	 * Returns null (with a reason in OutFailReason) if it couldn't.
	 */
	static UActorComponent* AddComponent(AActor* Actor, TSubclassOf<UActorComponent> ComponentClass, FText& OutFailReason);
};
