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
		TSharedPtr<IDetailTreeNode>* TreeNode = CategoryData->Get()->TreeNodes.Find(Property);
		
		FNodeWidgets Widgets = TreeNode->Get()->CreateNodeWidgets();

		TSharedPtr<SWidget> Row;
		
		if (Widgets.WholeRowWidget)
		{
			Row = Widgets.WholeRowWidget;
		}
		else
		{
			Row = SNew(SHorizontalBox)
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
		
		uint32 NumChildren;
		TArray<TSharedRef<IDetailTreeNode>> TreeNodeChildren;
		TreeNode->Get()->GetChildren(TreeNodeChildren);
		NumChildren = TreeNodeChildren.Num();
		
		if (PropertyHandle && PropertyHandle->IsValid())
		{
			TSharedPtr<SVerticalBox> PropertyAndChildrenBox = SNew(SVerticalBox);
			
			PropertyAndChildrenBox->AddSlot()
			[
				Row.ToSharedRef()
			];
			
			for (uint32 i = 0; i < NumChildren; ++i)
			{
				PropertyAndChildrenBox->AddSlot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						PropertyHandle->Get()->GetChildHandle(i).Get()->CreatePropertyNameWidget()
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						PropertyHandle->Get()->GetChildHandle(i).Get()->CreatePropertyValueWidgetWithCustomization(nullptr)
					]
				];
				
			}
			
			return PropertyAndChildrenBox;
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