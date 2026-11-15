#pragma once

#include "CoreMinimal.h"

class SConstraintCanvas;
class SLevelViewport;

/**
 * Shows a small "Double-click to edit" hint next to the cursor while it rests on an actor that has an Epith panel
 * (after UEpithEditorSettings::HoverHintDelay; off if the hint or Epith is disabled in the settings).
 * Ticked by the editor subsystem. Finds the actor under the cursor with the viewport's hit proxies (the same lookup the viewport
 * does to pick its cursor), and only does the panel check again when the cursor moves onto a different actor.
 */
class FEpithHoverHint : public TSharedFromThis<FEpithHoverHint>
{
public:
	FEpithHoverHint();

	~FEpithHoverHint();

	bool Tick(float DeltaTime);

	/** Removes the hint from its viewport. */
	void Hide();

private:
	/** The actor under the cursor and the viewport it's in, if the cursor is directly over a level viewport's scene. */
	AActor* FindHoveredActor(TSharedPtr<SLevelViewport>& OutViewport, FVector2D& OutLocalCursor) const;

	void Show(const TSharedRef<SLevelViewport>& Viewport);

	TSharedPtr<SConstraintCanvas> Canvas;

	TWeakPtr<SLevelViewport> AttachedViewport;

	// The one cached answer: the last actor the cursor was on, and whether it has a panel
	TWeakObjectPtr<AActor> LastActor;
	uint32 LastVersion = MAX_uint32;
	bool bLastHasPanel = false;

	double HoverStartTime = 0.0;

	/** Cursor position in the viewport, in Slate units; where the hint is drawn. */
	FVector2D LocalCursor = FVector2D::ZeroVector;
};
