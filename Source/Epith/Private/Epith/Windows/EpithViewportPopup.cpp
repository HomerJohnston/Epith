#include "Epith/Windows/EpithViewportPopup.h"

#include "Epith/EpithLog.h"
#include "Epith/Subsystems/EpithEditorSubsystem.h"
#include "Epith/Widgets/SEpithTabbedPane.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SSegmentedControl.h"

#define LOCTEXT_NAMESPACE "Epith"

void FEpithViewportPopup::Show(AActor* Target, FEpithClassDefinition* AttributeSet)
{
	FVector2D ScreenPos(800, 400);
	
	FSlateApplication::Get().PushMenu(
		FSlateApplication::Get().GetUserFocusedWidget(0).ToSharedRef(),
		FWidgetPath(), 
		BuildProperties(Target, AttributeSet),
		FDeprecateSlateVector2D(ScreenPos.X, ScreenPos.Y),
		FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup),
		true);
}

TSharedRef<SWidget> FEpithViewportPopup::BuildProperties(AActor* Target, FEpithClassDefinition* AttributeSet)
{
	return SNew(SBorder)
	.Padding(12)
	[
		SNew(SBox)
		[
			///BuildTabHeader({INVTEXT("Tab1"), INVTEXT("Tab2"), INVTEXT("Tab3")})
			SNew(SEpithTabbedPane)
			.Tabs( { INVTEXT("One"), INVTEXT("Two"), INVTEXT("Treefiddy") } )
			.Panes( { BuildDummyPane(INVTEXT("First Pane")), BuildDummyPane(INVTEXT("Second Pane")), BuildDummyPane(INVTEXT("Third Pane")) } )
		]
	];
}

TSharedRef<SWidget> FEpithViewportPopup::BuildTabHeader(TArray<FText> Tabs)
{
	TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox);
	
	for (FText Text : Tabs)
	{
		Box->AddSlot()
		[
			SNew(SCheckBox)
			[
				SNew(SBox)
			]
		];
	}
	
	return Box;
}

TSharedRef<SWidget> FEpithViewportPopup::BuildDummyPane(FText Text)
{
	return SNew(SBorder)
	.Padding(8)
	[
		SNew(SBox)
		.WidthOverride(120)
		.HeightOverride(240)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(INVTEXT("Row One"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(Text)				
			]
		]
	];
}


#undef LOCTEXT_NAMESPACE
