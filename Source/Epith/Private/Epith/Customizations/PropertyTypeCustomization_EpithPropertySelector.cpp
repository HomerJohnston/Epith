#include "Epith/Customizations/PropertyTypeCustomization_EpithPropertySelector.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"
#include "Epith/Misc/EpithPropertySelector.h"

void FPropertyTypeCustomization_EpithPropertySelector::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	ObjectProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEpithPropertySelector, Object));
	PropertyPathProperty = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEpithPropertySelector, PropertyPath));
	
	HeaderRow.NameContent()
	[
		ObjectProperty->CreatePropertyNameWidget()
	];
	
	HeaderRow.ValueContent()
	[
		ObjectProperty->CreatePropertyValueWidgetWithCustomization(nullptr)
	];
}

void FPropertyTypeCustomization_EpithPropertySelector::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	UObject* Object = nullptr;
	ObjectProperty->GetValue(Object);
	
	if (!Object)
	{
		return;
	}
		
	TSharedRef<SWidget> Selector = SNew(SComboButton)
		.OnGetMenuContent(this, &FPropertyTypeCustomization_EpithPropertySelector::OnGetMenuContent_PropertyName)
		.ContentPadding(0)
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text_Lambda([this] ()
			{
				FName CurrentValue;
				PropertyPathProperty->GetValue(CurrentValue);
				return FText::FromName(CurrentValue);
			} )
		];
	
	ChildBuilder.AddCustomRow(INVTEXT("None"))
	.NameContent()
	[
		PropertyPathProperty->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		Selector
	];
}

TSharedRef<SWidget> FPropertyTypeCustomization_EpithPropertySelector::OnGetMenuContent_PropertyName()
{	
	FMenuBuilder MenuBuilder(true, nullptr);
	
	UObject* Object = nullptr;
	ObjectProperty->GetValue(Object);
	
	FEpithPropertyDataContainer PropertyData(Object);

	using FClassPropertyHandles = TMap<FName, TSharedPtr<IPropertyHandle>>;
	
	TMap<UStruct*, FClassPropertyHandles> CategorizedPropertyMap;
	
	for (const auto&[Name, Handle] : PropertyData.GetPropertyHandles())
	{
		UStruct* OwnerType = Handle->GetProperty()->GetOwnerClass();
		
		if (!OwnerType)
		{
			OwnerType = Handle->GetProperty()->GetOwnerStruct();
		}

		if (!OwnerType)
		{
			continue;
		}
		
		FClassPropertyHandles& ClassPropertyHandles = CategorizedPropertyMap.FindOrAdd(OwnerType);
		
		ClassPropertyHandles.Add(Name, Handle);
	}
	
	CategorizedPropertyMap.KeySort([] (const UStruct& Left, const UStruct& Right)
	{
		return Left.IsChildOf(&Right);
	});
	
	for (auto&[Type, PropertyHandles] : CategorizedPropertyMap)
	{
		PropertyHandles.KeySort([](const FName& Left, const FName& Right)
		{
			return Left.Compare(Right) < 0;
		});
	}
	
	for (const auto&[Type, PropertyHandles] : CategorizedPropertyMap)
	{
		MenuBuilder.BeginSection(NAME_None, FText::FromName(Type->GetFName()));
		
		for (const auto&[Name, Handle] : PropertyHandles)
		{
			MenuBuilder.AddMenuEntry(FText::FromName(Name), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, Name]
			{
				PropertyPathProperty->SetValue(Name);
			} )));
		}
		
		MenuBuilder.EndSection();
	}
	
	return MenuBuilder.MakeWidget();
}
