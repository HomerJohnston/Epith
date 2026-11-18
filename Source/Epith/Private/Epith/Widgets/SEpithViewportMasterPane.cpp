#include "Epith/Widgets/SEpithViewportMasterPane.h"

#include "Editor.h"
#include "IAssetViewport.h"
#include "Selection.h"
#include "Epith/EpithLog.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/EditorWorldExtensions/EpithEditorWorldExtension.h"
#include "Epith/Style/EpithColor.h"
#include "Epith/Widgets/SEpithViewportMasterPaneHeader.h"
#include "Epith/Widgets/SEpithViewportObjectPane.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
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

	// The header bar along the top (the actor's name, pin and close buttons)
	static constexpr float MasterHeaderHeight = 40.0f;

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
				// The sections start below the whole header (MasterHeaderHeight), so the header doesn't cover the top of the first
				// section's header ribbon
				+ SOverlay::Slot()
				.Padding(0, MasterHeaderHeight - 8, 0, 0)
				[
					PaneBox.ToSharedRef()
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				[
					SNew(SBox)
					.HeightOverride(MasterHeaderHeight)
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

	USelection::SelectionChangedEvent.AddSP(this, &SEpithViewportMasterPane::OnSelectionChanged);

	RegisterActiveTimer(0.1f, FWidgetActiveTimerDelegate::CreateSP(this, &SEpithViewportMasterPane::CheckTargets));
}

SEpithViewportMasterPane::~SEpithViewportMasterPane()
{
	FCoreUObjectDelegates::OnObjectsReplaced.RemoveAll(this);
	USelection::SelectionChangedEvent.RemoveAll(this);

	OpenPanes.RemoveAll([this](const TWeakPtr<SEpithViewportMasterPane>& Pane)
	{
		return !Pane.IsValid() || Pane.Pin().Get() == this;
	});
}

FReply SEpithViewportMasterPane::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled();
}

FReply SEpithViewportMasterPane::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled();
}

FReply SEpithViewportMasterPane::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled();
}

FReply SEpithViewportMasterPane::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled();
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
	BuiltPaneSignature = MakePaneSignature(Target.Get());

	// Sections sit directly one under the other: each one's header ribbon marks where it starts
	for (const TSharedRef<SEpithViewportObjectPane>& ObjectPane : ObjectPanes)
	{
		PaneBox->AddSlot()
		.AutoHeight()
		[
			ObjectPane
		];
	}
}

SEpithViewportMasterPane::FPaneSignature SEpithViewportMasterPane::MakePaneSignature(UObject* InTarget)
{
	FPaneSignature Signature;

	for (const FEpithObjectPanelMatch& Match : UEpithEditorWorldExtension::FindObjectPanelMatches(InTarget, false))
	{
		Signature.Emplace(Match.Object->GetFName(), Match.PanelDefinition);
	}

	return Signature;
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

	// A component with a panel definition was added, removed or renamed (by a button, the Details panel, undo, a script...)
	const bool bPanesChanged = !bAnyPaneStale && MakePaneSignature(Target.Get()) != BuiltPaneSignature;

	if (bAnyPaneStale || bPanesChanged)
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

bool SEpithViewportMasterPane::IsTargetTheSelection() const
{
	USelection* SelectedActors = GEditor ? GEditor->GetSelectedActors() : nullptr;

	return SelectedActors && SelectedActors->Num() == 1 && SelectedActors->GetSelectedObject(0) == Target.Get();
}

void SEpithViewportMasterPane::SetPinned(bool bInPinned)
{
	bPinned = bInPinned;

	// While pinned, the selection may have moved on; unpinning applies the rule right away rather than at the next change
	if (!bPinned && !bClosed && Cast<AActor>(Target.Get()) && !IsTargetTheSelection())
	{
		Close();
	}
}

void SEpithViewportMasterPane::OnSelectionChanged(UObject* ChangedSelection)
{
	// Only the level's actor selection counts (not e.g. component or asset selections)
	if (bClosed || bPinned || !GEditor || ChangedSelection != GEditor->GetSelectedActors())
	{
		return;
	}

	// Panels for anything other than an actor aren't tied to the actor selection
	if (!Cast<AActor>(Target.Get()))
	{
		return;
	}

	// Checks the resulting selection rather than reacting to every event, so re-selecting the target (e.g. the click that
	// follows the double-click which opened the panel) doesn't close it
	if (!IsTargetTheSelection())
	{
		Close();
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

bool SEpithViewportMasterPane::IsOpenFor(const UObject* Object)
{
	for (const TWeakPtr<SEpithViewportMasterPane>& Pane : OpenPanes)
	{
		TSharedPtr<SEpithViewportMasterPane> Pinned = Pane.Pin();

		if (Pinned.IsValid() && !Pinned->bClosed && Pinned->Target.Get() == Object)
		{
			return true;
		}
	}

	return false;
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

FCursorReply SEpithViewportMasterPane::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	return FCursorReply::Cursor(EMouseCursor::Default);
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
