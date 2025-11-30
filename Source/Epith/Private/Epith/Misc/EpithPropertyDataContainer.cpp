#include "Epith/Misc/EpithPropertyDataContainer.h"

#include "IPropertyRowGenerator.h"
#include "Epith/EpithLog.h"

FEpithPropertyDataContainer::FEpithPropertyDataContainer(AActor* Target)
{
	DefaultCategory = Target->GetClass()->GetFName();
	
	Build(Target);
}

void FEpithPropertyDataContainer::Build(AActor* Actor)
{
	static const FName PropertyEditorName = "PropertyEditor";
	static const FName CategoryName = "TransformCommon";
	static const FName LocationName = "Location";
	static const FName RotationName = "Rotation";
	static const FName ScaleName = "Scale";

	FPropertyEditorModule& PropertyEditor = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>(PropertyEditorName);
	PropertyRowGenerator = PropertyEditor.CreatePropertyRowGenerator(FPropertyRowGeneratorArgs());
	PropertyRowGenerator->SetObjects( { Actor } );

	for (const TSharedRef<IDetailTreeNode>& CategoryNode : PropertyRowGenerator->GetRootTreeNodes())
	{
		TArray<TSharedRef<IDetailTreeNode>> ChildNodes;
		CategoryNode->GetChildren(ChildNodes);

		if (Categories.IsEmpty())
		{
			DefaultCategory = CategoryNode->GetNodeName();
		}
		
		TSharedPtr<FEpithPropertyCategoryContainer> CategoryData = Categories.Add(CategoryNode->GetNodeName(), MakeShared<FEpithPropertyCategoryContainer>());
		
		for (const TSharedRef<IDetailTreeNode>& ChildNode : ChildNodes)
		{	
			const FName NodeName = ChildNode->GetNodeName();

			TSharedPtr<IPropertyHandle> NewHandle = ChildNode->CreatePropertyHandle();
			ensure(NewHandle.IsValid() && NewHandle->IsValidHandle());
			
			CategoryData->TreeNodes.Add(NodeName, ChildNode);
			CategoryData->PropertyHandles.Add(NodeName, NewHandle);
			
			UE_LOG(LogEpith, Display, TEXT("Found, Category: %s, Property: %s"), *CategoryNode->GetNodeName().ToString(), *NodeName.ToString());
		}

		break;
	}
}
