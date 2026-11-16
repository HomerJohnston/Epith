#pragma once

#include "CoreMinimal.h"

#include "EpithSpecialControl.generated.h"

class AActor;
class SWidget;

/** What a special control is drawn for. Cheap to copy: controls can keep it in their widgets' callbacks. */
struct EPITH_API FEpithControlContext
{
	FEpithControlContext(UObject* InTarget, bool bInPreview);
	
	/** The object the panel is editing: an actor, or one of its components. */
	UObject* GetTarget() const;
	
	/** The target if it's an actor, otherwise the actor it belongs to (e.g. a component's owner). */
	AActor* GetActor() const;
	
	/**
	 * True in the panel designer's preview. The target is then a class default object, and the control must not change anything:
	 * draw it as it would look, but disabled.
	 */
	bool IsPreview() const;
	
private:
	TWeakObjectPtr<UObject> Target;
	bool bPreview = false;
};

/**
 * Base for special controls: custom widgets (buttons and so on) that a panel can show in a Special Control element.
 *
 * To make one, subclass this USTRUCT in C++ in an editor module that depends on Epith, and override Draw. Its UPROPERTYs
 * (EditDefaultsOnly) are its settings: they're edited in the panel definition and saved with it. Every subclass is found
 * automatically and listed in the panel designer and in the Special Control element's type picker. Give it a
 * DisplayName for a readable name. See FEpithSpecialControl_AddComponent for an example.
 */
USTRUCT(meta = (Hidden))
struct EPITH_API FEpithSpecialControl
{
	GENERATED_BODY()
	
	virtual ~FEpithSpecialControl() {}
	
	/** Builds the control's widget. Called each time the panel is drawn. */
	virtual TSharedPtr<SWidget> Draw(const FEpithControlContext& Context) const { return nullptr; }
};
