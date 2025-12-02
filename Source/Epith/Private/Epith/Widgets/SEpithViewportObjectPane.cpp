#include "Epith/Widgets/SEpithViewportObjectPane.h"

#include "Epith/EpithLog.h"
#include "Epith/Style/EpithColor.h"
#include "Epith/Style/EpithStyle.h"
#include "Epith/Widgets/EpithWindowElement.h"
#include "Epith/Widgets/SEpithViewportObjectPaneHeader.h"
#include "Styling/SlateIconFinder.h"

void SEpithViewportObjectPane::Construct(const FArguments& InArgs)
{
	if (!InArgs._Root || !InArgs._Target)
	{
		return;
	}
	
	Root = InArgs._Root;
	Target = InArgs._Target;

	FCoreUObjectDelegates::OnObjectsReplaced.AddSP(this, &SEpithViewportObjectPane::OnObjectsReinstanced);
	
	Build();
}

void SEpithViewportObjectPane::OnObjectsReinstanced(const TMap<UObject*, UObject*>& ReplacementMap)
{
	UObject* const* NewTarget = ReplacementMap.Find(Target.Get(true));
	
	if (NewTarget)
	{
		Target = *NewTarget;
		Build();
	}
}

void SEpithViewportObjectPane::Build()
{
	if (!Target.IsValid())
	{
		checkNoEntry();
		return;
	}

	UE_LOG(LogEpith, Display, TEXT("Rebuilding..."));
	
	PropertyData = MakeShared<FEpithPropertyDataContainer>(Target.Get());
			
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage( FAppStyle::GetBrush("Menu.Background") )
		.BorderBackgroundColor(EpithColor::White_BarelyTrans)
		.Padding(8)
		[
			SNew(SBox)
			.MinDesiredWidth(180)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(12, 12, 12, 8)
				[
					SNew(SEpithViewportObjectPaneHeader)
					.Target(Target.Get())
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					Root->Draw(PropertyData).ToSharedRef()
				]
			]
		]
	];
}
