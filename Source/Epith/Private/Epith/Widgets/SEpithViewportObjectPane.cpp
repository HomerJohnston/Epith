#include "Epith/Widgets/SEpithViewportObjectPane.h"

#include "Epith/EpithLog.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/Style/EpithColor.h"
#include "Epith/Style/EpithStyle.h"
#include "Epith/Widgets/EpithDrawContext.h"
#include "Epith/Widgets/EpithWindowElement.h"
#include "Epith/Widgets/SEpithViewportObjectPaneHeader.h"
#include "Styling/SlateIconFinder.h"

#define LOCTEXT_NAMESPACE "Epith"

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
	PropertyData.Reset();

	if (Target.IsValid())
	{
		PropertyData = MakeShared<FEpithPropertyDataContainer>(Target.Get(), ExpandedPaths);
		PropertyData->OnStructureChanged.AddSP(this, &SEpithViewportObjectPane::RequestRebuild);
	}

	Draw();
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
		Content = EpithDraw::DrawSlot(*PanelDefinition->GetRoot(), FEpithDrawContext(PropertyData));

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

		Box->AddSlot()
		.AutoHeight()
		.Padding(12, 12, 12, 8)
		[
			SNew(SEpithViewportObjectPaneHeader)
			.Target(Target.Get())
			.TitleOverride(Title)
		];
	}

	Box->AddSlot()
	.AutoHeight()
	[
		Content.ToSharedRef()
	];

	ChildSlot
	[
		// The Details panel's row color (Colors.Panel, #242424)
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
		.Padding(8)
		[
			SNew(SBox)
			.MinDesiredWidth(PanelDefinition.IsValid() ? PanelDefinition->GetMinWidth() : 180.0f)
			[
				Box
			]
		]
	];

	// Line up property columns on the first frame (no one-frame flicker)
	FEpithColumnLayout::MeasureNow(Box);
}

#undef LOCTEXT_NAMESPACE
