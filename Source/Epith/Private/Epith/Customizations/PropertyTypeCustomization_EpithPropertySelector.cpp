#include "Epith/Customizations/PropertyTypeCustomization_EpithPropertySelector.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"
#include "Epith/Misc/EpithPropertySelector.h"

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
				PropertyPathProperty->GetValue(CurrentValue);
				return FText::FromName(CurrentValue);
			} )
		];
	
	HeaderRow
	.NameContent()
	[
		PropertyPathProperty->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		Selector
	];
}

void FPropertyTypeCustomization_EpithPropertySelector::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

TSharedRef<SWidget> FPropertyTypeCustomization_EpithPropertySelector::OnGetMenuContent_PropertyName()
{	
	FMenuBuilder MenuBuilder(true, nullptr);
	
	const UClass* Class = PropertyPathProperty->GetOuterBaseClass();
	
	if (Class != UEpithPanelDefinition::StaticClass())
	{
		return SNew(STextBlock)
			.Text(INVTEXT("At this time, this struct only works when used in a child of UEpithPanelDefinition. Sorry!"));
	}

	TArray<UObject*> Outers;
	PropertyPathProperty->GetOuterObjects(Outers);
	
	if (Outers.Num() != 1)
	{
		return SNew(STextBlock)
			.Text(INVTEXT("Unknown error!"));
	}
	
	UEpithPanelDefinition* PanelDefinition = Cast<UEpithPanelDefinition>(Outers[0]);	
	
	FEpithPropertyDataContainer PropertyData(PanelDefinition->GetTargetActorType());

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
