#pragma once
#include "Widgets/Layout/Anchors.h"
#include "Widgets/Layout/SConstraintCanvas.h"

class SConstraintCanvas;

class SEpithViewportMasterPane : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SEpithViewportMasterPane)
		: _StartLocation(FVector2D(100, 100))
		{}

		SLATE_ARGUMENT(TSharedPtr<SWidget>, Canvas)

		SLATE_ARGUMENT(FVector2D, StartLocation)
		
		SLATE_ARGUMENT(TArray<TSharedPtr<SWidget>>, ChildPanes)
		
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	
	TWeakPtr<SWidget> Canvas;
	
	FReply StartDragging(FVector2D InTabGrabScreenSpaceOffset, const FPointerEvent& MouseEvent);
	
	void FinishDragging(const FVector2D InLocation);
	
	void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	
	void Close();
	
	FMargin GetOffset();
	
	bool bDragging = false;
	FVector2D CurrentPanelLocation;
	FVector2D StartDragCursorLocation;
	FVector2D CurrentDelta;
};
