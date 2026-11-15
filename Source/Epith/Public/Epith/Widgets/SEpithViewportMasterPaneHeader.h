#pragma once

class SEpithViewportMasterPane;

class SEpithViewportMasterPaneHeader : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SEpithViewportMasterPaneHeader) {}

		SLATE_ARGUMENT(TSharedPtr<SEpithViewportMasterPane>, Parent)
		
		SLATE_ATTRIBUTE(FText, Title)
		
	SLATE_END_ARGS()
	
	void Construct(const FArguments InArgs);
	
	TWeakPtr<SEpithViewportMasterPane> ParentPtr;
	
	// Dragging the header moves the whole panel. Uses mouse capture rather than a drag-drop operation,
	// so releasing the mouse anywhere (even outside the viewport) always ends the drag.
	FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	
	FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	
	FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	
	void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	
	/** The header is the drag handle, so it shows the four-way move cursor. */
	FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;
};
