#include "Epith/Widgets/SEpithViewportObjectPane.h"

#include "Epith/EpithLog.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/Style/EpithColor.h"
#include "Epith/Style/EpithStyle.h"
#include "Epith/Widgets/EpithDrawContext.h"
#include "Epith/Widgets/EpithWindowElement.h"
#include "Epith/Widgets/SEpithViewportObjectPaneHeader.h"
#include "Styling/SlateIconFinder.h"

#include "Containers/Ticker.h"

#define LOCTEXT_NAMESPACE "Epith"

namespace EpithViewportObjectPane
{
	/**
	 * Sections the user collapsed, for this editor session. Kept outside the pane, which is recreated whenever the panel
	 * rebuilds (a component is recreated, the panel is reopened...). See GetSectionKey.
	 */
	static TSet<FString> CollapsedSections;

	/** The panel definition and the target's name: the same section of the same actor (or component) after a rebuild. */
	static FString GetSectionKey(const UEpithPanelDefinition* Definition, const UObject* Target)
	{
		return FString::Printf(TEXT("%s|%s"), Definition ? *Definition->GetPathName() : TEXT(""), Target ? *Target->GetName() : TEXT(""));
	}
}

void SEpithViewportObjectPane::Construct(const FArguments& InArgs)
{
	if (!InArgs._PanelDefinition || !InArgs._Target)
	{
		return;
	}

	PanelDefinition.Reset(InArgs._PanelDefinition);
	Target = InArgs._Target;

	FCoreUObjectDelegates::OnObjectsReplaced.AddSP(this, &SEpithViewportObjectPane::OnObjectsReinstanced);
	FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(this, &SEpithViewportObjectPane::OnObjectPropertyChanged);

	Build();
}

SEpithViewportObjectPane::~SEpithViewportObjectPane()
{
	FCoreUObjectDelegates::OnObjectsReplaced.RemoveAll(this);
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);
}

void SEpithViewportObjectPane::OnObjectsReinstanced(const TMap<UObject*, UObject*>& ReplacementMap)
{
	bool bNeedsRebuild = false;

	// The panel definition itself can be replaced (e.g. its Blueprint class was recompiled, or the package was reloaded)
	if (UObject* const* NewDefinition = ReplacementMap.Find(PanelDefinition.Get()))
	{
		PanelDefinition.Reset(Cast<UEpithPanelDefinition>(*NewDefinition));
		bNeedsRebuild = true;
	}

	if (UObject* const* NewTarget = ReplacementMap.Find(Target.Get(true)))
	{
		Target = *NewTarget;
		bNeedsRebuild = true;
	}

	// Rebuilt right away, so the panel never shows a frame of rows for the object that's gone (they'd collapse or change
	// size for that frame: a flicker). Build() keeps the old row generator alive until it's safe to release.
	if (bNeedsRebuild)
	{
		Build();
	}
}

void SEpithViewportObjectPane::OnObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
	// Only rebuild when the layout changes. Rebuilding on changes to the target would tear down the widget being edited.
	if (Object && Object == PanelDefinition.Get())
	{
		Build();
	}
}

void SEpithViewportObjectPane::RequestRebuild()
{
	if (bRebuildPending)
	{
		return;
	}

	bRebuildPending = true;

	// Deferred: this is called from the row generator's tick, possibly while one of our widgets (e.g. an array's "+" button) is mid-click.
	RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateSP(this, &SEpithViewportObjectPane::HandleDeferredRebuild));
}

EActiveTimerReturnType SEpithViewportObjectPane::HandleDeferredRebuild(double InCurrentTime, float InDeltaTime)
{
	// A full Build() since the request already redrew everything
	if (!bRebuildPending)
	{
		return EActiveTimerReturnType::Stop;
	}

	bRebuildPending = false;

	RapidRebuildCount = (InCurrentTime - LastAutoRebuildTime < 0.25) ? RapidRebuildCount + 1 : 0;
	LastAutoRebuildTime = InCurrentTime;

	if (RapidRebuildCount == 20)
	{
		UE_LOG(LogEpith, Warning, TEXT("Epith panel for [%s] keeps getting row refreshes; ignoring them until things settle."), Target.IsValid() ? *Target->GetName() : TEXT("?"));
	}

	if (RapidRebuildCount >= 20)
	{
		return EActiveTimerReturnType::Stop;
	}

	// Let go of the old widgets first: rows the refresh kept will hand out the same widget instances again.
	ChildSlot
	[
		SNullWidget::NullWidget
	];

	// Keep the same row generator: re-read its new rows and redraw
	if (PropertyData.IsValid())
	{
		PropertyData->RegatherRows();
	}

	Draw();

	return EActiveTimerReturnType::Stop;
}

void SEpithViewportObjectPane::Build()
{
	// Always start from fresh property rows: a row's widgets can only be shown once, and the old ones belong to the tree we're replacing.
	if (PropertyData.IsValid())
	{
		// The old rows aren't released here but on the next core tick. Build() can run from inside the old row generator's own
		// tick (emptying an array reruns the actor's construction scripts, which replaces its components, which lands here),
		// and a generator mustn't be destroyed while it ticks. Creating the new one now is fine: the engine queues it.
		PropertyData->OnStructureChanged.RemoveAll(this);

		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Retired = MoveTemp(PropertyData)](float)
		{
			// One-shot: the ticker drops this delegate, and the old rows with it
			return false;
		}));

		PropertyData.Reset();
	}

	// Any redraw the old rows asked for is covered by this one
	bRebuildPending = false;

	if (Target.IsValid())
	{
		PropertyData = MakeShared<FEpithPropertyDataContainer>(Target.Get(), ExpandedPaths);
		PropertyData->OnStructureChanged.AddSP(this, &SEpithViewportObjectPane::RequestRebuild);
	}

	Draw();
}

bool SEpithViewportObjectPane::IsExpanded() const
{
	// Not foldable: always open (even if it was collapsed while it was foldable)
	if (!PanelDefinition.IsValid() || !PanelDefinition->IsFoldable())
	{
		return true;
	}

	return !EpithViewportObjectPane::CollapsedSections.Contains(EpithViewportObjectPane::GetSectionKey(PanelDefinition.Get(), Target.Get()));
}

void SEpithViewportObjectPane::SetExpanded(bool bExpanded)
{
	const FString Key = EpithViewportObjectPane::GetSectionKey(PanelDefinition.Get(), Target.Get());

	if (bExpanded)
	{
		EpithViewportObjectPane::CollapsedSections.Remove(Key);
	}
	else
	{
		EpithViewportObjectPane::CollapsedSections.Add(Key);
	}
}

void SEpithViewportObjectPane::Draw()
{
	TSharedPtr<SWidget> Content;

	if (!Target.IsValid())
	{
		Content = SNew(STextBlock).Text(LOCTEXT("ObjectPane_TargetGone", "This object no longer exists."));
	}
	else if (!PanelDefinition.IsValid() || !PanelDefinition->GetRoot())
	{
		Content = SNew(STextBlock).Text(LOCTEXT("ObjectPane_NoLayout", "This panel definition has no layout."));
	}
	else
	{
		FEpithDrawContext Context(PropertyData);
		Context.ViewState = ViewState;

		Content = EpithDraw::DrawSlot(*PanelDefinition->GetRoot(), Context);

		if (!Content.IsValid())
		{
			Content = SNew(STextBlock).Text(LOCTEXT("ObjectPane_EmptyLayout", "This panel's layout is empty."));
		}
	}

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	if (Target.IsValid())
	{
		// The definition's Title if it has one. Otherwise a supplementary panel is titled after its own target class (e.g. "Actor"),
		// so it doesn't repeat the main panel's title; a main panel falls back to the object's class or component name.
		FText Title = PanelDefinition.IsValid() ? PanelDefinition->GetTitle() : FText::GetEmpty();

		if (Title.IsEmpty() && PanelDefinition.IsValid() && PanelDefinition->IsSupplementary() && PanelDefinition->GetTargetClass())
		{
			Title = PanelDefinition->GetTargetClass()->GetDisplayNameText();
		}

		TSharedRef<SWidget> Header = SNew(SEpithViewportObjectPaneHeader)
			.Target(Target.Get())
			.TitleOverride(Title);

		// The header sits on a medium-gray ribbon running the full width of the section, flush with its top edge
		if (!PanelDefinition.IsValid() || !PanelDefinition->IsFoldable())
		{
			Box->AddSlot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(EpithViewportObjectPane::HeaderRibbonColor)
				.Padding(FMargin(20, 6, 20, 6))
				[
					Header
				]
			];
		}
		else
		{
			// Foldable: clicking the ribbon folds the section away (a little lighter while hovered, so it reads as clickable),
			// and an arrow shows which way it is
			TSharedPtr<SBorder> Ribbon;

			Box->AddSlot()
			.AutoHeight()
			[
				SAssignNew(Ribbon, SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.Padding(FMargin(4, 6, 20, 6))
				.Cursor(EMouseCursor::Hand)
				.ToolTipText(LOCTEXT("ObjectPane_FoldTooltip", "Click to collapse or expand this section"))
				.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& MouseEvent)
				{
					if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
					{
						return FReply::Unhandled();
					}

					SetExpanded(!IsExpanded());
					return FReply::Handled();
				})
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SImage)
						.Image_Lambda([this]()
						{
							return FAppStyle::GetBrush(IsExpanded() ? "TreeArrow_Expanded" : "TreeArrow_Collapsed");
						})
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					.Padding(4, 0, 0, 0)
					[
						Header
					]
				]
			];

			TWeakPtr<SBorder> WeakRibbon = Ribbon;

			Ribbon->SetBorderBackgroundColor(TAttribute<FSlateColor>::CreateLambda([WeakRibbon]()
			{
				TSharedPtr<SBorder> PinnedRibbon = WeakRibbon.Pin();
				const bool bHovered = PinnedRibbon.IsValid() && PinnedRibbon->IsHovered();

				return FSlateColor(bHovered ? EpithViewportObjectPane::HeaderRibbonHoverColor : EpithViewportObjectPane::HeaderRibbonColor);
			}));
		}
	}

	// Collapsed along with its padding, so a folded section is just its header
	Box->AddSlot()
	.AutoHeight()
	[
		SNew(SBox)
		.Padding(8)
		.Visibility_Lambda([this]()
		{
			return IsExpanded() ? EVisibility::Visible : EVisibility::Collapsed;
		})
		[
			Content.ToSharedRef()
		]
	];

	ChildSlot
	[
		// The Details panel's row color (Colors.Panel, #242424). No padding of its own: the header ribbon reaches its edges,
		// and the content is padded instead.
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
		.Padding(0)
		[
			SNew(SBox)
			.WidthOverride(PanelDefinition.IsValid() ? PanelDefinition->GetWidth() : 360.0f)
			[
				Box
			]
		]
	];

	// Line up property columns on the first frame (no one-frame flicker)
	FEpithColumnLayout::MeasureNow(Box);
}

#undef LOCTEXT_NAMESPACE
