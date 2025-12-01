#include "Epith/Windows/EpithViewportPopup.h"

#include "IDetailTreeNode.h"
#include "IPropertyRowGenerator.h"
#include "Epith/EpithLog.h"
#include "Epith/Subsystems/EpithEditorSubsystem.h"
#include "Epith/Widgets/EpithWindowElement.h"
#include "Epith/Widgets/SEpithActorPane.h"
#include "Epith/Widgets/SEpithTabbedPane.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SSegmentedControl.h"

#define LOCTEXT_NAMESPACE "Epith"

bool FEpithViewportPopup::Show(AActor* Target, const FEpithWindowElement& RootWindowElement)
{
	if (!IsValid(Target))
	{
		return false;
	}
	
	FVector2D ScreenPos(800, 400);
	
	TSharedPtr<FEpithPropertyDataContainer> PropertyData = MakeShared<FEpithPropertyDataContainer>(Target);
	
	FText Title = FText::FromString(Target->GetActorLabel() + " Properties");
	
	TSharedPtr<SWidget> Widget = SNew(SEpithActorPane)
		.Title(Title)
		.PropertyData(PropertyData)
		.Root(&RootWindowElement);
	
	if (!Widget)
	{
		return false;
	}
	
	FSlateApplication::Get().PushMenu(
		FSlateApplication::Get().GetUserFocusedWidget(0).ToSharedRef(),
		FWidgetPath(),
		Widget.ToSharedRef(),
		FDeprecateSlateVector2D(ScreenPos.X, ScreenPos.Y),
		FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup),
		true);
	
	return true;
}

#undef LOCTEXT_NAMESPACE
