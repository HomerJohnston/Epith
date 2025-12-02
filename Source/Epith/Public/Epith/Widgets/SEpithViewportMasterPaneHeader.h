#pragma once

class SEpithViewportMasterPane;

class SEpithViewportMasterPaneHeader : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SEpithViewportMasterPaneHeader) {}

		SLATE_ARGUMENT(TSharedPtr<SEpithViewportMasterPane>, Parent)
		
		SLATE_ARGUMENT(FText, Title)
		
	SLATE_END_ARGS()
	
	void Construct(const FArguments InArgs);
	
	TWeakPtr<SEpithViewportMasterPane> ParentPtr;
	
	FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	
	FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;;
};
