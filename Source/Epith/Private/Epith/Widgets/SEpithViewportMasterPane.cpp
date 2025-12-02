#include "Epith/Widgets/SEpithViewportMasterPane.h"

#include "IAssetViewport.h"
#include "LevelEditor.h"
#include "Epith/EpithLog.h"
#include "Epith/Style/EpithColor.h"
#include "Epith/Widgets/SEpithViewportMasterPaneHeader.h"
#include "Viewports/InViewportUIDragOperation.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SPopup.h"

void SEpithViewportMasterPane::Construct(const FArguments& InArgs)
{
	Canvas = InArgs._Canvas;
	CurrentPanelLocation = InArgs._StartLocation;
	
	check(Canvas.IsValid());
	
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
	
	for (int32 i = 0; i < InArgs._ChildPanes.Num(); ++i)
	{
		TSharedPtr<SWidget> ChildPane = InArgs._ChildPanes[i];
		
		Box->AddSlot()
		.AutoHeight()
		[
			ChildPane.ToSharedRef()
		];
		
		if (i < InArgs._ChildPanes.Num() - 1)
		{
			Box->AddSlot()
			.AutoHeight()
			[
				SNew(SSpacer)
				.Size(8)
			];
		}
	}

	TSharedRef<SEpithViewportMasterPane> Self = StaticCastSharedRef<SEpithViewportMasterPane>(AsShared());

	ChildSlot
	[
		SNew(SBackgroundBlur)
		.BlurStrength(4)
		.Padding(0.0f)
		.CornerRadius(FVector4(4.0f, 4.0f, 4.0f, 4.0f))
		[
			SNew(SBorder)
			.BorderImage( FAppStyle::GetBrush("Menu.Background") )
			.BorderBackgroundColor(EpithColor::White_Glass)
			.Padding(0)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				.Padding(0, 24, 0, 0)
				[
					Box
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				//.Padding(-8, -8, -8, 0)
				[
					SNew(SBox)
					.HeightOverride(40)
					[
						SNew(SEpithViewportMasterPaneHeader)
						.Parent(Self)
						.Title(INVTEXT("Properties Panel"))
					]
				]
			]
		]
	];
}

FReply SEpithViewportMasterPane::StartDragging(FVector2D InTabGrabScreenSpaceOffset, const FPointerEvent& MouseEvent)
{
	FOnInViewportUIDropped OnUIDropped = FOnInViewportUIDropped::CreateSP(this, &SEpithViewportMasterPane::FinishDragging);
	
	// This is ghetto but whatever it's an editor tool.
	TSharedRef<SWidget> DummyDragger = SNew(SImage).DesiredSizeOverride(FVector2D(1,1));
	
	// Start dragging.
	TSharedRef<FInViewportUIDragOperation> DragDropOperation =
		FInViewportUIDragOperation::New(
			DummyDragger,
			InTabGrabScreenSpaceOffset,
			GetDesiredSize(),
			OnUIDropped
		);
	
	bDragging = true;
	StartDragCursorLocation = FSlateApplication::Get().GetCursorPos();
	
	SetRenderOpacity(0.8);
	
	return FReply::Handled().BeginDragDrop(DragDropOperation);
}

void SEpithViewportMasterPane::FinishDragging(const FVector2D InLocation)
{
	this->SetRenderOpacity(1.0f);
	
	bDragging = false;
	CurrentPanelLocation = CurrentPanelLocation += CurrentDelta;
	
	SetRenderOpacity(1.0);
	
	ULevelEditorViewportSettings* LevelEditorViewportSettings = GetMutableDefault<ULevelEditorViewportSettings>();
	LevelEditorViewportSettings->LastInViewportMenuLocation = InLocation;
	LevelEditorViewportSettings->SaveConfig();
}

void SEpithViewportMasterPane::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	
	if (bDragging)
	{
		FVector2D CursorPos = FSlateApplication::Get().GetCursorPos();
		
		CurrentDelta = CursorPos - StartDragCursorLocation;
	}
	else
	{
		CurrentDelta = FVector2D::ZeroVector;
	}
}

void SEpithViewportMasterPane::Close()
{
	static const FName LevelEditorName("LevelEditor");
	if (FModuleManager::Get().IsModuleLoaded(LevelEditorName))
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(LevelEditorName);
		TSharedPtr<IAssetViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveViewport();
		if (ActiveLevelViewport.IsValid())
		{
			ActiveLevelViewport->RemoveOverlayWidget(Canvas.Pin().ToSharedRef());
		}
	}
}

FMargin SEpithViewportMasterPane::GetOffset()
{
	FVector2D Size = GetPaintSpaceGeometry().GetAbsoluteSize();
	
	FVector2D Position;
	
	if (bDragging)
	{
		Position = CurrentPanelLocation + CurrentDelta;
	}
	else
	{
		Position = CurrentPanelLocation;
	}
	
	return FMargin(Position.X, Position.Y, Size.X, Size.Y);
}
