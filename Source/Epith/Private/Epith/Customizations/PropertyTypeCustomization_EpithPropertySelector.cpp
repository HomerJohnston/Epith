#include "Epith/Customizations/PropertyTypeCustomization_EpithPropertySelector.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Epith/EpithLog.h"
#include "Epith/Editor/EpithPropertyMenu.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"
#include "Epith/Misc/EpithPropertySelector.h"

#define LOCTEXT_NAMESPACE "Epith"

void FPropertyTypeCustomization_EpithPropertySelector::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	PropertyPathProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEpithPropertySelector, PropertyPath));
	
	TSharedRef<SWidget> Selector = SNew(SComboButton)
		.OnGetMenuContent(this, &FPropertyTypeCustomization_EpithPropertySelector::OnGetMenuContent_PropertyName)
		.ContentPadding(0)
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text_Lambda([this] ()
			{
				FName CurrentValue;
				
				if (!PropertyPathProperty.IsValid() || PropertyPathProperty->GetValue(CurrentValue) != FPropertyAccess::Success)
				{
					return LOCTEXT("PropertySelector_MultipleValues", "Multiple Values");
				}
				
				if (CurrentValue.IsNone())
				{
					return LOCTEXT("PropertySelector_None", "Select a property...");
				}
				
				return FText::FromName(CurrentValue);
			} )
		];
	
	HeaderRow
	.NameContent()
	[
		PropertyPathProperty->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(200)
	[
		Selector
	];
}

void FPropertyTypeCustomization_EpithPropertySelector::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

TSharedRef<SWidget> FPropertyTypeCustomization_EpithPropertySelector::OnGetMenuContent_PropertyName()
{	
	TArray<UObject*> Outers;
	PropertyPathProperty->GetOuterObjects(Outers);
	
	if (Outers.Num() != 1)
	{
		return EpithPropertyMenu::MakeMessage(LOCTEXT("PropertySelector_MultipleOuters", "Edit one panel definition at a time to pick a property."));
	}
	
	const UEpithPanelDefinition* PanelDefinition = Cast<UEpithPanelDefinition>(Outers[0]);
	
	if (!PanelDefinition)
	{
		return EpithPropertyMenu::MakeMessage(LOCTEXT("PropertySelector_WrongOuter", "This property selector only works inside an Epith panel definition."));
	}
	
	TSharedPtr<IPropertyHandle> PathHandle = PropertyPathProperty;
	
	return EpithPropertyMenu::Make(PanelDefinition->GetTargetClass(), [PathHandle](FName Path)
	{
		PathHandle->SetValue(Path);
	});
}

#undef LOCTEXT_NAMESPACE
