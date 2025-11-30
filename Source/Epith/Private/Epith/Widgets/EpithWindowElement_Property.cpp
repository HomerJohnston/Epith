#include "Epith/Widgets/EpithWindowElement_Property.h"

#include "Epith/Misc/EpithPropertyDataContainer.h"

#define LOCTEXT_NAMESPACE "Epith"

TSharedPtr<SWidget> FEpithWindowElement_Property::Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const
{
	FName __Category = Category;
	
	if (__Category == NAME_None)
	{
		__Category = InPropertyData->DefaultCategory;
	}

	TSharedPtr<FEpithPropertyCategoryContainer>* CategoryData = InPropertyData.Get()->Categories.Find(__Category);
	
	if (CategoryData)
	{
		TSharedPtr<IPropertyHandle>* PropertyHandle = CategoryData->Get()->PropertyHandles.Find(Property);
		
		if (PropertyHandle && PropertyHandle->IsValid())
		{
			return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				PropertyHandle->Get()->CreatePropertyNameWidget()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				PropertyHandle->Get()->CreatePropertyValueWidgetWithCustomization(nullptr)
			];
		}
		else
		{
			return SNew(STextBlock)
				.Text(LOCTEXT("Epith_MissingPropertyCategory", "MISSING PROPERTY"));	
		}
	}
	else
	{
		return SNew(STextBlock)
			.Text(LOCTEXT("Epith_MissingPropertyCategory", "MISSING CAT"));	
	}
}

#undef LOCTEXT_NAMESPACE