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
				// Pin: keeps the panel open when the selection changes. Pulled left by its own padding, so its icon sits as far from
				// the left edge as the close button's does from the right.
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(-8, 0, 0, 0)
				[
					SNew(SButton)
					.Cursor(EMouseCursor::Default)
					.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
					.ContentPadding(8)
					.NormalPaddingOverride(FMargin(0))
					.ToolTipText_Lambda([this]()
					{
						TSharedPtr<SEpithViewportMasterPane> PinnedParent = ParentPtr.Pin();

						return PinnedParent.IsValid() && PinnedParent->IsPinned()
							? LOCTEXT("ViewportPropertyEditor_Unpin", "Pinned: stays open when the selection changes. Click to unpin.")
							: LOCTEXT("ViewportPropertyEditor_Pin", "Pin: keep this panel open when the selection changes.");
					})
					.OnClicked_Lambda([this]()
					{
						if (TSharedPtr<SEpithViewportMasterPane> PinnedParent = ParentPtr.Pin())
						{
							PinnedParent->SetPinned(!PinnedParent->IsPinned());
						}
						return FReply::Handled();
					})
					[
						SNew(SImage)
						.DesiredSizeOverride(FVector2D(16, 16))
						.ColorAndOpacity(FSlateColor::UseForeground())
						.Image_Lambda([this]()
						{
							TSharedPtr<SEpithViewportMasterPane> PinnedParent = ParentPtr.Pin();
							const bool bPinned = PinnedParent.IsValid() && PinnedParent->IsPinned();
							return FAppStyle::Get().GetBrush(bPinned ? "Icons.Pinned" : "Icons.Unpinned");
						})
					]
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(InArgs._Title)
					.Font(EpithFonts.Font_ViewportPopup_PanelTitle)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0, 0, 0, 0)
				[
					SNew(SButton)
					.Cursor(EMouseCursor::Default) // A normal arrow over the close button, not the header's move cursor
					.ToolTipText(LOCTEXT("ViewportPropertyEditor_CloseButton", "Close")) // TODO it'd be nice to properly support OK/CANCEL ... bit of a major change, would have to make a dummy copy and reflect it back into the source
					.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
					.ContentPadding(8)
					.NormalPaddingOverride(FMargin(0))
					.OnClicked_Lambda( [this] ()
					{
						if (TSharedPtr<SEpithViewportMasterPane> PinnedParent = ParentPtr.Pin())
						{
							PinnedParent->Close();
						}
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

FReply SEpithViewportMasterPaneHeader::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	TSharedPtr<SEpithViewportMasterPane> PinnedParent = ParentPtr.Pin();

	if (!PinnedParent.IsValid())
	{
		return FReply::Unhandled();
	}

	PinnedParent->BeginDrag(MouseEvent.GetScreenSpacePosition());

	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SEpithViewportMasterPaneHeader::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!HasMouseCapture())
	{
		return FReply::Unhandled();
	}

	if (TSharedPtr<SEpithViewportMasterPane> PinnedParent = ParentPtr.Pin())
	{
		PinnedParent->UpdateDrag(MouseEvent.GetScreenSpacePosition());
	}

	return FReply::Handled();
}

FReply SEpithViewportMasterPaneHeader::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!HasMouseCapture() || MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	if (TSharedPtr<SEpithViewportMasterPane> PinnedParent = ParentPtr.Pin())
	{
		PinnedParent->EndDrag();
	}

	return FReply::Handled().ReleaseMouseCapture();
}

FCursorReply SEpithViewportMasterPaneHeader::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	return FCursorReply::Cursor(EMouseCursor::CardinalCross);
}

void SEpithViewportMasterPaneHeader::OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	if (TSharedPtr<SEpithViewportMasterPane> PinnedParent = ParentPtr.Pin())
	{
		PinnedParent->EndDrag();
	}
}

#undef LOCTEXT_NAMESPACE