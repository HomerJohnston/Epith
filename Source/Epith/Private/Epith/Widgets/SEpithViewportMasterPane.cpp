#include "Epith/Widgets/SEpithViewportMasterPane.h"

#include "IAssetViewport.h"
#include "LevelEditor.h"
#include "Epith/Style/EpithColor.h"
#include "Epith/Widgets/SEpithViewportMasterPaneHeader.h"
#include "Viewports/InViewportUIDragOperation.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SPopup.h"

void SEpithViewportMasterPane::Construct(const FArguments& InArgs)
{
	Owner = InArgs._Owner;
	
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

	Canvas = SNew(SConstraintCanvas);
	
	ActualPane = SNew(SBackgroundBlur)
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
		];
	
	Canvas->AddSlot()
	.Alignment(FVector2D(0, 0))
	.Anchors(FAnchors(0, 0))
	.Offset(FMargin(400, 200, 600, 600))
	//.AutoSize(true)
	[
		ActualPane.ToSharedRef()
	];
	
	ChildSlot
	[
		Canvas.ToSharedRef()	
	];
}

FReply SEpithViewportMasterPane::StartDragging(FVector2D InTabGrabScreenSpaceOffset, const FPointerEvent& MouseEvent)
{
	UE_LOG(LogTemp, Display, TEXT("Start: %s"), *InTabGrabScreenSpaceOffset.ToString());
	
	FOnInViewportUIDropped OnUIDropped = FOnInViewportUIDropped::CreateSP(this, &SEpithViewportMasterPane::FinishDragging);
	
	// Start dragging.
	TSharedRef<FInViewportUIDragOperation> DragDropOperation =
		FInViewportUIDragOperation::New(
			ActualPane.ToSharedRef(),
			InTabGrabScreenSpaceOffset,
			GetDesiredSize(),
			OnUIDropped
		);
	
	return FReply::Handled().BeginDragDrop(DragDropOperation);
}

void SEpithViewportMasterPane::FinishDragging(const FVector2D InLocation)
{
	UE_LOG(LogTemp, Display, TEXT("To: %s"), *InLocation.ToString());
	
	this->SetRenderOpacity(1.0f);
	
	ULevelEditorViewportSettings* LevelEditorViewportSettings = GetMutableDefault<ULevelEditorViewportSettings>();
	LevelEditorViewportSettings->LastInViewportMenuLocation = InLocation;
	LevelEditorViewportSettings->SaveConfig();
}

void SEpithViewportMasterPane::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
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
			ActiveLevelViewport->RemoveOverlayWidget(AsShared());
		}
	}
}

FAnchors SEpithViewportMasterPane::GetAnchors()
{
	return FAnchors( FMath::RandRange(0.0f, 0.2f), FMath::RandRange(0.0f, 0.2f), FMath::RandRange(0.0f, 0.2f), FMath::RandRange(0.0f, 0.2f) );
}
