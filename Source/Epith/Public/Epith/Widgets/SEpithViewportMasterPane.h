#pragma once

#include "Widgets/SCompoundWidget.h"

class UEpithPanelDefinition;

class IAssetViewport;
class SEpithViewportObjectPane;
class SVerticalBox;

/**
 * The floating window shown in a level viewport. Holds one SEpithViewportObjectPane per matching object (the target actor and its components).
 * Lives on an SConstraintCanvas that is added to the viewport as an overlay widget.
 */
class SEpithViewportMasterPane : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SEpithViewportMasterPane)
		: _StartLocation(FVector2D(100, 100))
		{}

		/** The canvas this pane sits on. The canvas is what gets added to / removed from the viewport. */
		SLATE_ARGUMENT(TSharedPtr<SWidget>, Canvas)

		/** The viewport the canvas was added to as an overlay. Used to remove it again on Close. */
		SLATE_ARGUMENT(TSharedPtr<IAssetViewport>, Viewport)

		/** The object (normally an actor) the panel was opened for. Object panes are built for it and its subobjects. */
		SLATE_ARGUMENT(UObject*, Target)

		/** Used if no panel has been dragged yet this editor session; otherwise the last panel position is reused. */
		SLATE_ARGUMENT(FVector2D, StartLocation)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/**
	 * The whole panel is solid to the mouse: anything its own controls don't use stops here instead of falling through to the
	 * level viewport behind it (which would select or deselect actors, fly the camera, and so on). Controls inside the panel
	 * still get every event first.
	 */
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	~SEpithViewportMasterPane();

	/** False if no panel definition matched the target or any of its subobjects. */
	bool HasPanes() const { return ObjectPanes.Num() > 0; }

	/** Removes this pane's canvas from its viewport. */
	void Close();

	/** Closes every open Epith panel (e.g. when Play-in-Editor starts). */
	static void CloseAll();

	/** True if a panel is open for this object. */
	static bool IsOpenFor(const UObject* Object);

	// Dragging, driven by SEpithViewportMasterPaneHeader. Positions are in screen space.
	void BeginDrag(const FVector2D& ScreenPosition);

	void UpdateDrag(const FVector2D& ScreenPosition);

	void EndDrag();

	/** Offset for the canvas slot, in canvas-local units. */
	FMargin GetOffset() const;

	/**
	 * The normal arrow over the whole panel, instead of the viewport's crosshair showing through.
	 * Widgets inside that want their own cursor (text boxes, the header's move cursor) still get it, because Slate asks them first.
	 */
	FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;

	/** The actor's label (or the object's name), shown in the header. */
	FText GetTitle() const;

	/**
	 * An unpinned panel closes as soon as the selection changes so that its target is no longer the one selected actor
	 * (another actor is clicked, the target is deselected, more actors are shift-selected). A pinned panel stays open.
	 */
	bool IsPinned() const { return bPinned; }

	/** Unpinning closes the panel straight away if its target is no longer the one selected actor. */
	void SetPinned(bool bInPinned);

protected:
	/** (Re)creates the object panes for Target and its subobjects. */
	void RebuildPanes();

	/** Polls for deleted targets and recreated components. Runs as an active timer, outside of Slate's paint pass, so it can safely close or rebuild. */
	EActiveTimerReturnType CheckTargets(double InCurrentTime, float InDeltaTime);

	void OnObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap);

	/** Closes an unpinned panel when the actor selection no longer is just the target. */
	void OnSelectionChanged(UObject* ChangedSelection);

	/** True if exactly one actor is selected and it's the target. */
	bool IsTargetTheSelection() const;

	/** Keeps the pane fully inside the canvas (i.e. the viewport). */
	FVector2D ClampToCanvas(const FVector2D& Location) const;

	TWeakPtr<SWidget> Canvas;

	TWeakPtr<IAssetViewport> Viewport;

	TWeakObjectPtr<UObject> Target;

	TSharedPtr<SVerticalBox> PaneBox;

	TArray<TSharedRef<SEpithViewportObjectPane>> ObjectPanes;

	/** One entry per pane: the object's name and its panel definition. Names, not pointers: Blueprint construction scripts recreate components under the same names. */
	using FPaneSignature = TArray<TPair<FName, TWeakObjectPtr<UEpithPanelDefinition>>>;

	/** The panes the panel was last built with. When the target's panes would differ (a component was added or removed), the panel rebuilds. */
	FPaneSignature BuiltPaneSignature;

	static FPaneSignature MakePaneSignature(UObject* Target);

	bool bClosed = false;

	bool bPinned = false;

	bool bDragging = false;

	/** Canvas-local position of the pane's top-left corner. */
	FVector2D CurrentPanelLocation = FVector2D::ZeroVector;

	FVector2D DragStartScreenPosition = FVector2D::ZeroVector;

	FVector2D DragStartPanelLocation = FVector2D::ZeroVector;

	static TArray<TWeakPtr<SEpithViewportMasterPane>> OpenPanes;

	/** Where the last panel was dragged to or closed at, for this editor session. */
	static TOptional<FVector2D> LastPanelLocation;
};
