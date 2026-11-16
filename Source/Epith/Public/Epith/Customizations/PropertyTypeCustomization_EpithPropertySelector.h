#pragma once

#include "IPropertyTypeCustomization.h"

class UEpithPanelDefinition;

class FPropertyTypeCustomization_EpithPropertySelector : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShared<FPropertyTypeCustomization_EpithPropertySelector>(); }

	/** For details views that edit a struct directly (no panel definition object to find the target class through). */
	static TSharedRef<IPropertyTypeCustomization> MakeInstanceFor(TWeakObjectPtr<UEpithPanelDefinition> InDefinition)
	{
		TSharedRef<FPropertyTypeCustomization_EpithPropertySelector> Customization = MakeShared<FPropertyTypeCustomization_EpithPropertySelector>();
		Customization->DefinitionOverride = InDefinition;
		return Customization;
	}

	/** The panel definition to use instead of the edited object, if set. */
	TWeakObjectPtr<UEpithPanelDefinition> DefinitionOverride;

	TSharedPtr<IPropertyHandle> PropertyPathProperty;

	void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

protected:
	TSharedRef<SWidget> OnGetMenuContent_PropertyName();
};