#pragma once
#include "Widgets/Layout/Anchors.h"

class SConstraintCanvas;

class SEpithViewportMasterPane : public SCompoundWidget
{
	
	SLATE_BEGIN_ARGS(SEpithViewportMasterPane) {}

		SLATE_ARGUMENT(TSharedPtr<SWidget>, Owner)
		
		SLATE_ARGUMENT(TArray<TSharedPtr<SWidget>>, ChildPanes)
		
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs);
	
	TSharedPtr<SWidget> Owner;
	
	TSharedPtr<SConstraintCanvas> Canvas;
	
	TSharedPtr<SWidget> ActualPane;
	
	FReply StartDragging(FVector2D InTabGrabScreenSpaceOffset, const FPointerEvent& MouseEvent);
	
	void FinishDragging(const FVector2D InLocation);
	
	void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	
	void Close();
	
	FAnchors GetAnchors();
};
