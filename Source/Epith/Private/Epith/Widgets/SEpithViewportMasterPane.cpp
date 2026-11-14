#include "Epith/Widgets/SEpithViewportMasterPane.h"

#include "IAssetViewport.h"
#include "Epith/EpithLog.h"
#include "Epith/EditorWorldExtensions/EpithEditorWorldExtension.h"
#include "Epith/Style/EpithColor.h"
#include "Epith/Widgets/SEpithViewportMasterPaneHeader.h"
#include "Epith/Widgets/SEpithViewportObjectPane.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/Layout/SSpacer.h"

TArray<TWeakPtr<SEpithViewportMasterPane>> SEpithViewportMasterPane::OpenPanes;
TOptional<FVector2D> SEpithViewportMasterPane::LastPanelLocation;

void SEpithViewportMasterPane::Construct(const FArguments& InArgs)
{
	Canvas = InArgs._Canvas;
	Viewport = InArgs._Viewport;
	Target = InArgs._Target;
	CurrentPanelLocation = LastPanelLocation.Get(InArgs._StartLocation);

	check(Canvas.IsValid());

	PaneBox = SNew(SVerticalBox);

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
					PaneBox.ToSharedRef()
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				[
					SNew(SBox)
					.HeightOverride(40)
					[
						SNew(SEpithViewportMasterPaneHeader)
						.Parent(Self)
						.Title(this, &SEpithViewportMasterPane::GetTitle)
					]
				]
			]
		]
	];

	RebuildPanes();

	OpenPanes.Add(Self);

	FCoreUObjectDelegates::OnObjectsReplaced.AddSP(this, &SEpithViewportMasterPane::OnObjectsReplaced);

	RegisterActiveTimer(0.1f, FWidgetActiveTimerDelegate::CreateSP(this, &SEpithViewportMasterPane::CheckTargets));
}

SEpithViewportMasterPane::~SEpithViewportMasterPane()
{
	FCoreUObjectDelegates::OnObjectsReplaced.RemoveAll(this);

	OpenPanes.RemoveAll([this](const TWeakPtr<SEpithViewportMasterPane>& Pane)
	{
		return !Pane.IsValid() || Pane.Pin().Get() == this;
	});
}

FText SEpithViewportMasterPane::GetTitle() const
{
	const UObject* PinnedTarget = Target.Get();
	
	if (const AActor* TargetActor = Cast<AActor>(PinnedTarget))
	{
		return FText::FromString(TargetActor->GetActorLabel());
	}
	
	return PinnedTarget ? FText::FromName(PinnedTarget->GetFName()) : FText::GetEmpty();
}

void SEpithViewportMasterPane::RebuildPanes()
{
	PaneBox->ClearChildren();
	ObjectPanes.Reset();

	if (!Target.IsValid())
	{
		return;
	}

	ObjectPanes = UEpithEditorWorldExtension::CreateObjectPanes(Target.Get());

	for (int32 i = 0; i < ObjectPanes.Num(); ++i)
	{
		PaneBox->AddSlot()
		.AutoHeight()
		[
			ObjectPanes[i]
		];

		if (i < ObjectPanes.Num() - 1)
		{
			PaneBox->AddSlot()
			.AutoHeight()
			[
				SNew(SSpacer)
				.Size(8)
			];
		}
	}
}

EActiveTimerReturnType SEpithViewportMasterPane::CheckTargets(double InCurrentTime, float InDeltaTime)
{
	if (bClosed)
	{
		return EActiveTimerReturnType::Stop;
	}

	// The actor was deleted, or its level was unloaded
	if (!Target.IsValid())
	{
		Close();
		return EActiveTimerReturnType::Stop;
	}

	// Blueprint actors destroy and recreate their construction-script components whenever a property changes.
	// Panes for the old components would edit dead objects, so rebuild against the new ones.
	const bool bAnyPaneStale = ObjectPanes.ContainsByPredicate([](const TSharedRef<SEpithViewportObjectPane>& Pane)
	{
		return !Pane->HasValidTarget();
	});

	if (bAnyPaneStale)
	{
		RebuildPanes();

		if (!HasPanes())
		{
			Close();
			return EActiveTimerReturnType::Stop;
		}
	}

	return EActiveTimerReturnType::Continue;
}

void SEpithViewportMasterPane::OnObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap)
{
	// Blueprint recompiles replace the actor instance; follow it rather than closing the panel.
	if (UObject* const* NewTarget = ReplacementMap.Find(Target.Get(true)))
	{
		Target = *NewTarget;
		RebuildPanes();
	}
}

void SEpithViewportMasterPane::Close()
{
	if (bClosed)
	{
		return;
	}

	bClosed = true;
	bDragging = false;

	LastPanelLocation = CurrentPanelLocation;

	TSharedPtr<IAssetViewport> PinnedViewport = Viewport.Pin();
	TSharedPtr<SWidget> PinnedCanvas = Canvas.Pin();

	if (PinnedViewport.IsValid() && PinnedCanvas.IsValid())
	{
		PinnedViewport->RemoveOverlayWidget(PinnedCanvas.ToSharedRef());
	}
}

void SEpithViewportMasterPane::CloseAll()
{
	// Copy first; closing destroys panes, which removes them from OpenPanes
	TArray<TSharedPtr<SEpithViewportMasterPane>> PanesToClose;

	for (const TWeakPtr<SEpithViewportMasterPane>& Pane : OpenPanes)
	{
		if (TSharedPtr<SEpithViewportMasterPane> Pinned = Pane.Pin())
		{
			PanesToClose.Add(Pinned);
		}
	}

	for (const TSharedPtr<SEpithViewportMasterPane>& Pane : PanesToClose)
	{
		Pane->Close();
	}
}

void SEpithViewportMasterPane::BeginDrag(const FVector2D& ScreenPosition)
{
	bDragging = true;
	DragStartScreenPosition = ScreenPosition;
	DragStartPanelLocation = ClampToCanvas(CurrentPanelLocation);

	SetRenderOpacity(0.8f);
}

void SEpithViewportMasterPane::UpdateDrag(const FVector2D& ScreenPosition)
{
	TSharedPtr<SWidget> PinnedCanvas = Canvas.Pin();

	if (!bDragging || !PinnedCanvas.IsValid())
	{
		return;
	}

	// Convert both points into the canvas's local space, so the pane tracks the cursor at any DPI scale
	const FGeometry& CanvasGeometry = PinnedCanvas->GetCachedGeometry();

	const FVector2D LocalDelta = FVector2D(CanvasGeometry.AbsoluteToLocal(ScreenPosition)) - FVector2D(CanvasGeometry.AbsoluteToLocal(DragStartScreenPosition));

	CurrentPanelLocation = ClampToCanvas(DragStartPanelLocation + LocalDelta);
}

void SEpithViewportMasterPane::EndDrag()
{
	if (!bDragging)
	{
		return;
	}

	bDragging = false;

	SetRenderOpacity(1.0f);

	LastPanelLocation = CurrentPanelLocation;
}

FMargin SEpithViewportMasterPane::GetOffset() const
{
	// Clamp every frame too, so the pane stays on screen when the viewport is resized
	const FVector2D Position = ClampToCanvas(CurrentPanelLocation);

	// The canvas slot is AutoSize, so only the position matters
	return FMargin(Position.X, Position.Y, 0, 0);
}

FVector2D SEpithViewportMasterPane::ClampToCanvas(const FVector2D& Location) const
{
	TSharedPtr<SWidget> PinnedCanvas = Canvas.Pin();

	if (!PinnedCanvas.IsValid())
	{
		return Location;
	}

	const FVector2D CanvasSize = PinnedCanvas->GetCachedGeometry().GetLocalSize();

	// Before the first layout pass the canvas has no size yet; don't clamp against zero
	if (CanvasSize.X <= 0.0 || CanvasSize.Y <= 0.0)
	{
		return Location;
	}

	const FVector2D PaneSize = GetDesiredSize();

	const double MaxX = FMath::Max(0.0, CanvasSize.X - PaneSize.X);
	const double MaxY = FMath::Max(0.0, CanvasSize.Y - PaneSize.Y);

	return FVector2D(FMath::Clamp(Location.X, 0.0, MaxX), FMath::Clamp(Location.Y, 0.0, MaxY));
}
