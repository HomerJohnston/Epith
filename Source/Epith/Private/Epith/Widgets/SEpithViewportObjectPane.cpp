#include "Epith/Widgets/SEpithViewportObjectPane.h"

#include "Epith/EpithLog.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/Style/EpithColor.h"
#include "Epith/Style/EpithStyle.h"
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

void SEpithViewportObjectPane::Build()
{
	TSharedPtr<SWidget> Content;
	
	// Always start from fresh property rows: a row's widgets can only be shown once, and the old ones belong to the tree we're replacing.
	PropertyData.Reset();
	
	if (Target.IsValid())
	{
		PropertyData = MakeShared<FEpithPropertyDataContainer>(Target.Get());
	}

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
		Content = PanelDefinition->GetRoot()->Get<FEpithWindowElement>().Draw(PropertyData);

		if (!Content.IsValid())
		{
			Content = SNew(STextBlock).Text(LOCTEXT("ObjectPane_EmptyLayout", "This panel's layout is empty."));
		}
	}

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

	if (Target.IsValid())
	{
		Box->AddSlot()
		.AutoHeight()
		.Padding(12, 12, 12, 8)
		[
			SNew(SEpithViewportObjectPaneHeader)
			.Target(Target.Get())
			.TitleOverride(PanelDefinition.IsValid() ? PanelDefinition->GetTitle() : FText::GetEmpty())
		];
	}

	Box->AddSlot()
	.AutoHeight()
	[
		Content.ToSharedRef()
	];

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage( FAppStyle::GetBrush("Menu.Background") )
		.BorderBackgroundColor(EpithColor::White_BarelyTrans)
		.Padding(8)
		[
			SNew(SBox)
			.MinDesiredWidth(PanelDefinition.IsValid() ? PanelDefinition->GetMinWidth() : 180.0f)
			[
				Box
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE
