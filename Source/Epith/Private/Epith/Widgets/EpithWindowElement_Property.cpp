#include "Epith/Widgets/EpithWindowElement_Property.h"

#include "Engine/BlueprintGeneratedClass.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"
#include "Styling/StyleColors.h"

#define LOCTEXT_NAMESPACE "Epith"

TSharedPtr<SWidget> FEpithWindowElement_Property::Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const
{
	const FName PropertyPath = PropertySelector.GetPropertyPath();
	
	if (PropertyPath.IsNone())
	{
		return nullptr;
	}
	
	TSharedPtr<IDetailTreeNode> TreeNode = InPropertyData->FindTreeNode(PropertyPath);
	
	if (!TreeNode.IsValid())
	{
		return SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("PropertyElement_Missing", "Missing property: {0}"), FText::FromName(PropertyPath)))
			.ColorAndOpacity(FSlateColor(EStyleColor::Warning));
	}
	
	// A property row's widgets can only live in one place. Its type customization (e.g. the color picker) keeps pointers to the
	// widgets it made, so we must show exactly the widgets from the one CreateNodeWidgets() call and never generate a second set.
	// (The old code also called IDetailPropertyRow::GetDefaultWidgets, which re-runs the customization's CustomizeHeader and
	// repoints it at a second, hidden set of widgets; that is what made color pickers crash unless the hidden set stayed alive.)
	if (!InPropertyData->TryClaimPath(PropertyPath))
	{
		return SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("PropertyElement_Duplicate", "{0} is already shown in this panel"), FText::FromName(PropertyPath)))
			.ColorAndOpacity(FSlateColor(EStyleColor::Warning));
	}
	
	FNodeWidgets Widgets = TreeNode->CreateNodeWidgets();
	
	TSharedPtr<SWidget> RowWidget;
	
	if (Widgets.WholeRowWidget.IsValid())
	{
		RowWidget = Widgets.WholeRowWidget;
	}
	else if (!Widgets.NameWidget.IsValid() && !Widgets.ValueWidget.IsValid())
	{
		return SNew(STextBlock)
			.Text(FText::Format(LOCTEXT("PropertyElement_NoWidgets", "{0} has no widgets to show"), FText::FromName(PropertyPath)))
			.ColorAndOpacity(FSlateColor(EStyleColor::Warning));
	}
	else
	{
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
		
		if (Widgets.NameWidget.IsValid())
		{
			Row->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 8, 0)
			[
				Widgets.NameWidget.ToSharedRef()
			];
		}
		
		if (Widgets.ValueWidget.IsValid())
		{
			Row->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				Widgets.ValueWidget.ToSharedRef()
			];
		}
		
		RowWidget = Row;
	}
	
	// The details system includes EditDefaultsOnly properties (and Blueprint variables that aren't Instance Editable) in the rows,
	// but they aren't meant to be changed on placed instances. Show them read-only, the way the class defaults would look.
	const FProperty* EditBlocker = FEpithPropertyDataContainer::FindInstanceEditBlocker(InPropertyData->FindPropertyHandle(PropertyPath));
	
	if (EditBlocker && InPropertyData->IsEditingInstance())
	{
		const bool bIsBlueprintVariable = Cast<UBlueprintGeneratedClass>(EditBlocker->GetOwnerClass()) != nullptr;
		
		const FText Reason = bIsBlueprintVariable
			? FText::Format(LOCTEXT("PropertyElement_ReadOnlyBlueprint", "Read-only: {0} isn't Instance Editable. To edit it here, open the Blueprint and click the eye icon next to the variable."), EditBlocker->GetDisplayNameText())
			: FText::Format(LOCTEXT("PropertyElement_ReadOnlyDefaults", "Read-only: {0} is EditDefaultsOnly, so it can only be changed on the class defaults."), EditBlocker->GetDisplayNameText());
		
		return SNew(SBox)
			.IsEnabled(false)
			.ToolTipText(Reason)
			[
				RowWidget.ToSharedRef()
			];
	}
	
	return RowWidget;
}

#undef LOCTEXT_NAMESPACE
