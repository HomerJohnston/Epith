#include "EpithHoverHint.h"

#include "EngineUtils.h"
#include "EpithPanelDefinitionCache.h"
#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "Editor.h"
#include "Epith/Settings/EpithEditorSettings.h"
#include "Epith/Widgets/SEpithViewportMasterPane.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SceneViewport.h"
#include "Styling/AppStyle.h"
#include "Widgets/SViewport.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "Epith"

FEpithHoverHint::FEpithHoverHint()
{
}

FEpithHoverHint::~FEpithHoverHint()
{
	Hide();
}

bool FEpithHoverHint::Tick(float DeltaTime)
{
	const UEpithEditorSettings* Settings = UEpithEditorSettings::Get();
	
	if (!Settings->IsHoverHintEnabled() || !FSlateApplication::IsInitialized() || (GEditor && GEditor->IsPlaySessionInProgress()))
	{
		LastActor.Reset();
		Hide();
		return true;
	}

	TSharedPtr<SLevelViewport> Viewport;
	AActor* Actor = FindHoveredActor(Viewport, LocalCursor);

	const double Now = FPlatformTime::Seconds();
	const uint32 Version = EpithPanelDefinitionCache::GetVersion();

	if (Actor != LastActor.Get())
	{
		LastActor = Actor;
		LastVersion = Version;
		bLastHasPanel = EpithPanelDefinitionCache::HasAnyPanel(Actor);
		HoverStartTime = Now;
	}
	else if (Version != LastVersion)
	{
		// A definition changed while the cursor stayed put; check again without restarting the delay
		LastVersion = Version;
		bLastHasPanel = EpithPanelDefinitionCache::HasAnyPanel(Actor);
	}

	const bool bShow = Actor && Viewport.IsValid()
		&& bLastHasPanel
		&& Now - HoverStartTime >= Settings->HoverHintDelay
		&& FSlateApplication::Get().GetPressedMouseButtons().Num() == 0
		&& !SEpithViewportMasterPane::IsOpenFor(Actor);

	if (bShow)
	{
		Show(Viewport.ToSharedRef());
	}
	else
	{
		Hide();
	}

	return true;
}

AActor* FEpithHoverHint::FindHoveredActor(TSharedPtr<SLevelViewport>& OutViewport, FVector2D& OutLocalCursor) const
{
	FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>("LevelEditor");
	TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule ? LevelEditorModule->GetFirstLevelEditor() : nullptr;

	if (!LevelEditor.IsValid())
	{
		return nullptr;
	}

	FSlateApplication& SlateApp = FSlateApplication::Get();
	const FVector2D CursorPos = SlateApp.GetCursorPos();

	// The topmost widget under the cursor. Only the bare viewport counts: over its toolbar, an open Epith panel, a menu or
	// another window, the cursor isn't on the scene.
	const FWidgetPath Path = SlateApp.LocateWindowUnderMouse(CursorPos, SlateApp.GetInteractiveTopLevelWindows());

	if (!Path.IsValid())
	{
		return nullptr;
	}

	const FArrangedWidget& Leaf = Path.Widgets.Last();

	for (const TSharedPtr<SLevelViewport>& LevelViewport : LevelEditor->GetViewports())
	{
		if (!LevelViewport.IsValid())
		{
			continue;
		}

		TSharedPtr<FSceneViewport> SceneViewport = LevelViewport->GetSharedActiveViewport();

		if (!SceneViewport.IsValid() || SceneViewport->GetViewportWidget().Pin().Get() != &Leaf.Widget.Get() || !SceneViewport->IsCursorVisible())
		{
			continue;
		}

		// Hit proxies are looked up in viewport pixels
		const FVector2D LocalSlate = Leaf.Geometry.AbsoluteToLocal(CursorPos);
		const FVector2D LocalPixels = LocalSlate * Leaf.Geometry.GetAccumulatedLayoutTransform().GetScale();
		const FIntPoint Size = SceneViewport->GetSizeXY();

		const int32 X = FMath::FloorToInt32(LocalPixels.X);
		const int32 Y = FMath::FloorToInt32(LocalPixels.Y);

		if (X < 0 || Y < 0 || X >= Size.X || Y >= Size.Y)
		{
			return nullptr;
		}

		HActor* ActorProxy = HitProxyCast<HActor>(SceneViewport->GetHitProxy(X, Y));
		AActor* Actor = ActorProxy ? ActorProxy->Actor.Get() : nullptr;

		if (!IsValid(Actor))
		{
			return nullptr;
		}

		OutViewport = LevelViewport;
		OutLocalCursor = LocalSlate;
		return Actor;
	}

	return nullptr;
}

void FEpithHoverHint::Show(const TSharedRef<SLevelViewport>& Viewport)
{
	if (AttachedViewport.Pin() == Viewport)
	{
		return;
	}

	Hide();

	if (!Canvas.IsValid())
	{
		Canvas = SNew(SConstraintCanvas)
			.Visibility(EVisibility::HitTestInvisible);
	}

	// Built fresh each time, so the slot reads the cursor position from this hint (it outlives the canvas slot)
	TWeakPtr<FEpithHoverHint> WeakThis = AsShared();

	Canvas->ClearChildren();
	Canvas->AddSlot()
	.Anchors(FAnchors(0, 0))
	.Alignment(FVector2D(0, 0))
	.AutoSize(true)
	.Offset_Lambda([WeakThis]()
	{
		TSharedPtr<FEpithHoverHint> Pinned = WeakThis.Pin();
		// Below and to the right of the arrow, like a tooltip
		return Pinned.IsValid() ? FMargin(Pinned->LocalCursor.X + 14, Pinned->LocalCursor.Y + 20, 0, 0) : FMargin(0);
	})
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Menu.Background"))
		.Padding(FMargin(6, 3))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("HoverHint", "Double-click to edit"))
			.Font(FAppStyle::GetFontStyle("SmallFont"))
		]
	];

	Viewport->AddOverlayWidget(Canvas.ToSharedRef());
	AttachedViewport = Viewport;
}

void FEpithHoverHint::Hide()
{
	TSharedPtr<SLevelViewport> Viewport = AttachedViewport.Pin();

	if (Viewport.IsValid() && Canvas.IsValid())
	{
		Viewport->RemoveOverlayWidget(Canvas.ToSharedRef());
	}

	AttachedViewport.Reset();
}

#undef LOCTEXT_NAMESPACE
