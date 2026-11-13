#include "Epith/Widgets/EpithWindowElement_Property.h"

#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
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
	
	FDetailWidgetRow NewRow;
	TSharedPtr<IDetailPropertyRow> DetailPropertyRow = TreeNode->GetRow();
	TSharedPtr<SWidget> NameWidget;
	TSharedPtr<SWidget> ValueWidget;
	DetailPropertyRow->GetDefaultWidgets(NameWidget, ValueWidget, NewRow, true);
	
	if (PropertyHandle->GetPropertyDisplayName().ToString().Contains("Color"))
	{
		FWidgetPath Path;
		FSlateApplication::Get().GeneratePathToWidgetUnchecked(NameWidget.ToSharedRef(), Path);
		UE_LOG(LogEpith, Display, TEXT("Path to canvas: %s"), *Path.ToString());
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
			/*
			SNew(SButton)
			.OnClicked_Lambda( [Widgets] ()
			{
				FWidgetPath Path;
				FSlateApplication::Get().GeneratePathToWidgetChecked(Widgets.ValueWidget->GetAllChildren()[0].GetSlotAt(0).GetWidget()->GetAllChildren()[0].GetSlotAt(0).GetWidget()->GetAllChildren()[0].GetSlotAt(0).GetWidget()->GetAllChildren()[0].GetSlotAt(0).GetWidget()->GetAllChildren()[0].GetSlotAt(0).GetWidget(), Path);
				UE_LOG(LogEpith, Display, TEXT("Path to canvas: %s"), *Path.ToString());
				return FReply::Handled();
			})
			[
				Widgets.NameWidget.ToSharedRef()
			]
			*/
			NameWidget.ToSharedRef()
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			// What is this??? This is a dirty-ass hack. I discovered that color pickers CRASH because they can't find this widget when I instead display Widgets.ValueWidget
			// This widget is the raw property widget and Widgets.ValueWidget uses the property type customization for the property, so I want the other one!
			// I also found that changing the Visibility of ValueWidget caused it to break. It needs to be rendered and kept alive or something.
			SNew(SBox)
			.WidthOverride(0)
			.RenderOpacity(0.0f)
			[
				ValueWidget.ToSharedRef()
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			Widgets.ValueWidget.ToSharedRef()
		];
	}
	else
	{
		Row = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0, 0, 8, 0)
		[
			NameWidget.ToSharedRef()
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			ValueWidget.ToSharedRef()
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
	
	/*
	PropertyAndChildrenBox->AddSlot()
	[
		Widgets.NameWidget.ToSharedRef()
	];
	PropertyAndChildrenBox->AddSlot()
	[
		Widgets.ValueWidget.ToSharedRef()
	];*/
	
#if 0
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
				ChildPropertyHandle->CreatePropertyValueWidgetWithCustomization(nullptr)
				//ChildWidgets.ValueWidget.ToSharedRef()
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
#endif
	
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
