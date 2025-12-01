#pragma once

#include "IPropertyTypeCustomization.h"

class FPropertyTypeCustomization_EpithPropertySelector : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShared<FPropertyTypeCustomization_EpithPropertySelector>(); }
	
	TSharedPtr<IPropertyHandle> PropertyPathProperty;
	
	void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

	void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	
protected:
	TSharedRef<SWidget> OnGetMenuContent_PropertyName();
};