#include "Epith/Widgets/SEpithViewportMasterPaneHeader.h"

#include "Epith/Style/EpithStyle.h"
#include "Epith/Widgets/SEpithViewportMasterPane.h"

#define LOCTEXT_NAMESPACE "Epith"

void SEpithViewportMasterPaneHeader::Construct(const FArguments InArgs)
{
	ParentPtr = InArgs._Parent;
	
	ChildSlot
	[
		SNew(SBox)
		.Padding(-8, -8, -8, 0)
		[
			SNew(SBorder)
			.BorderImage(FEpithStyle::GetImageBrush(EpithBrushes.Background_ViewportMasterPanelHeader))
			.Padding(12, 4, 4, 12)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				[
					SNew(STextBlock)
					.Text(InArgs._Title)	
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0, 0, 0, 0)
				[
					SNew(SButton)
					.ToolTipText(LOCTEXT("ViewportPropertyEditor_CloseButton", "Close")) // TODO it'd be nice to properly support OK/CANCEL ... bit of a major change, would have to make a dummy copy and reflect it back into the source
					.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
					.ContentPadding(8)
					.NormalPaddingOverride(FMargin(0))
					.OnClicked_Lambda( [this] ()
					{
						ParentPtr.Pin()->Close();
						return FReply::Handled();
					})
					[
						SNew(SImage)
						.DesiredSizeOverride(FVector2D(16, 16))
						.Image(FAppStyle::Get().GetBrush("Icons.X"))
					]
				]
			]
		]
	];
}

FReply SEpithViewportMasterPaneHeader::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Need to remember where within a tab we grabbed
	const FVector2D TabGrabScreenSpaceOffset = MouseEvent.GetScreenSpacePosition() - MyGeometry.GetAbsolutePosition();

	TSharedPtr<SEpithViewportMasterPane> PinnedParent = ParentPtr.Pin();
	if (PinnedParent.IsValid())
	{
		return PinnedParent->StartDragging(/*TabGrabScreenSpaceOffset*/MouseEvent.GetScreenSpacePosition(), MouseEvent);
	}

	return FReply::Unhandled();
	return SCompoundWidget::OnDragDetected(MyGeometry, MouseEvent);
}

FReply SEpithViewportMasterPaneHeader::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
}

#undef LOCTEXT_NAMESPACE