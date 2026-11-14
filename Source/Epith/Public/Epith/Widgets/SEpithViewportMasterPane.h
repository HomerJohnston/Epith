#pragma once

#include "Widgets/SCompoundWidget.h"

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

	~SEpithViewportMasterPane();

	/** False if no panel definition matched the target or any of its subobjects. */
	bool HasPanes() const { return ObjectPanes.Num() > 0; }

	/** Removes this pane's canvas from its viewport. */
	void Close();

	/** Closes every open Epith panel (e.g. when Play-in-Editor starts). */
	static void CloseAll();

	// Dragging, driven by SEpithViewportMasterPaneHeader. Positions are in screen space.
	void BeginDrag(const FVector2D& ScreenPosition);

	void UpdateDrag(const FVector2D& ScreenPosition);

	void EndDrag();

	/** Offset for the canvas slot, in canvas-local units. */
	FMargin GetOffset() const;

	/** The actor's label (or the object's name), shown in the header. */
	FText GetTitle() const;

protected:
	/** (Re)creates the object panes for Target and its subobjects. */
	void RebuildPanes();

	/** Polls for deleted targets and recreated components. Runs as an active timer, outside of Slate's paint pass, so it can safely close or rebuild. */
	EActiveTimerReturnType CheckTargets(double InCurrentTime, float InDeltaTime);

	void OnObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap);

	/** Keeps the pane fully inside the canvas (i.e. the viewport). */
	FVector2D ClampToCanvas(const FVector2D& Location) const;

	TWeakPtr<SWidget> Canvas;

	TWeakPtr<IAssetViewport> Viewport;

	TWeakObjectPtr<UObject> Target;

	TSharedPtr<SVerticalBox> PaneBox;

	TArray<TSharedRef<SEpithViewportObjectPane>> ObjectPanes;

	bool bClosed = false;

	bool bDragging = false;

	/** Canvas-local position of the pane's top-left corner. */
	FVector2D CurrentPanelLocation = FVector2D::ZeroVector;

	FVector2D DragStartScreenPosition = FVector2D::ZeroVector;

	FVector2D DragStartPanelLocation = FVector2D::ZeroVector;

	static TArray<TWeakPtr<SEpithViewportMasterPane>> OpenPanes;

	/** Where the last panel was dragged to or closed at, for this editor session. */
	static TOptional<FVector2D> LastPanelLocation;
};
