#include "Epith/Widgets/EpithWindowElement_Property.h"

#include "Epith/Misc/EpithPropertyDataContainer.h"

#define LOCTEXT_NAMESPACE "Epith"

TSharedPtr<SWidget> FEpithWindowElement_Property::Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const
{
	TSharedPtr<IPropertyHandle> PropertyHandle = InPropertyData->FindPropertyHandle(PropertySelector.GetPropertyPath());
	TSharedPtr<IDetailTreeNode> TreeNode = InPropertyData->FindTreeNode(PropertySelector.GetPropertyPath());
	
	if (!PropertyHandle.IsValid() || !TreeNode.IsValid())
	{
		return SNew(STextBlock)
			.Text(LOCTEXT("Epith_MissingPropertyCategory", "NULL PROPERTY"));	
	}
	
	FNodeWidgets Widgets = TreeNode->CreateNodeWidgets();

	TSharedPtr<SWidget> Row = nullptr;
	
	if (Widgets.WholeRowWidget)
	{
		Row = Widgets.WholeRowWidget;
	}
	else if (Widgets.NameWidget && Widgets.ValueWidget)
	{
		Row = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0, 0, 8, 0)
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			PropertyHandle->CreatePropertyValueWidgetWithCustomization(nullptr)
		];
	}
	
	if (!Row)
	{
		return SNew(STextBlock).Text(INVTEXT("ERROR - No widgets!"));
	}
	
	TSharedPtr<SVerticalBox> PropertyAndChildrenBox = SNew(SVerticalBox);
	PropertyAndChildrenBox->AddSlot()
	.AutoHeight()
	[
		Row.ToSharedRef()
	];
	
	uint32 NumChildren;
	TArray<TSharedRef<IDetailTreeNode>> TreeNodeChildren;
	TreeNode->GetChildren(TreeNodeChildren);
	NumChildren = TreeNodeChildren.Num();
	
	for (uint32 i = 0; i < NumChildren; ++i)
	{
		FNodeWidgets ChildWidgets = TreeNodeChildren[i]->CreateNodeWidgets();
		TSharedPtr<IPropertyHandle> ChildPropertyHandle = TreeNodeChildren[i]->CreatePropertyHandle();
		TSharedPtr<SWidget> ChildRow = nullptr;
		
		if (ChildWidgets.WholeRowWidget)
		{
			ChildRow = ChildWidgets.WholeRowWidget;
		}
		else if (ChildWidgets.NameWidget && ChildWidgets.ValueWidget)
		{
			ChildRow = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0, 0, 8, 0)
			[
				ChildWidgets.NameWidget.ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.5)
			[
				ChildWidgets.ValueWidget.ToSharedRef()
			];
		}
		else
		{
			continue;
		}
		
		PropertyAndChildrenBox->AddSlot()
		.AutoHeight()
		[
			ChildRow.ToSharedRef()
		];
	}
	
	
	return PropertyAndChildrenBox;
}

void FEpithWindowElement_Property::RecursiveDrawRowChildren(TSharedPtr<FEpithPropertyDataContainer> InPropertyData, TSharedPtr<IDetailTreeNode> TreeNode, TArray<TSharedRef<SWidget>>& Widgets)
{
	/*
	FName __Category = Category;
	
	if (__Category == NAME_None)
	{
		__Category = InPropertyData->DefaultCategory;
	}

	TSharedPtr<FEpithPropertyCategoryContainer>* CategoryData = InPropertyData.Get()->Categories.Find(__Category);
	
	if (CategoryData)
	{
		// Draw this guy
		TSharedPtr<IPropertyHandle>* PropertyHandle = CategoryData->Get()->PropertyHandles.Find(Property);
		
		FNodeWidgets NodeWidgets = TreeNode->CreateNodeWidgets();

		TSharedPtr<SWidget> Row;
		
		if (NodeWidgets.WholeRowWidget)
		{
			Row = NodeWidgets.WholeRowWidget;
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
		
		if (Row)
		{
			Widgets.Add(Row.ToSharedRef());
		}
		
		// Draw children
		TArray<TSharedRef<IDetailTreeNode>> TreeNodeChildren;
		TreeNode->GetChildren(TreeNodeChildren);
		
		for (TSharedRef<IDetailTreeNode> Child : TreeNodeChildren)
		{
			RecursiveDrawRowChildren(InPropertyData, Child, Widgets);
		}
	}
	*/	
}

#undef LOCTEXT_NAMESPACE
