#include "Epith/Misc/EpithPropertyDataContainer.h"

#include "IPropertyRowGenerator.h"
#include "Epith/EpithLog.h"

FEpithPropertyDataContainer::FEpithPropertyDataContainer(UObject* Target)
{
	Build(Target);
}

void FEpithPropertyDataContainer::Build(UObject* Actor)
{
	static const FName PropertyEditorName = "PropertyEditor";

	FPropertyEditorModule& PropertyEditor = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>(PropertyEditorName);
	PropertyRowGenerator = PropertyEditor.CreatePropertyRowGenerator(FPropertyRowGeneratorArgs());
	PropertyRowGenerator->SetObjects( { Actor } );

	TArray<TSharedRef<IDetailTreeNode>> TreeNodes;

	TArray<TSharedRef<IDetailTreeNode>> PendingTreeNodes;
	
	PendingTreeNodes = PropertyRowGenerator->GetRootTreeNodes();

	TArray<FName> CategoryStack;
	
	while (PendingTreeNodes.Num() > 0)
	{
		const TSharedRef<IDetailTreeNode> PoppedNode = PendingTreeNodes.Pop(EAllowShrinking::No);
		
		GatherChildDetailTreeNodesRecursive(PoppedNode, PendingTreeNodes);
		
		if (PoppedNode->GetNodeType() == EDetailNodeType::Item)
		{
			TSharedPtr<IPropertyHandle> PropertyHandle = PoppedNode->CreatePropertyHandle();
			
			if (!PropertyHandle)
			{
				UE_LOG(LogEpith, Display, TEXT("Null property handle for IDetailTreeNode: %s"), *PoppedNode->GetNodeName().ToString());
				continue;
			}
			// TODO does GeneratePathToProperty ever generate invalid characters?
			FName PathToProperty = FName(PropertyHandle->GeneratePathToProperty());
			
			AllTreeNodes.Add(PathToProperty, PoppedNode);
			AllPropertyHandles.Add(PathToProperty, PropertyHandle);
			
			TreeNodes.Add(PoppedNode);
		}
	}
	
	UE_LOG(LogEpith, Display, TEXT("All done"));
}

void FEpithPropertyDataContainer::GatherChildDetailTreeNodesRecursive(TSharedRef<IDetailTreeNode> ParentTreeNode, TArray<TSharedRef<IDetailTreeNode>>& PendingTreeNodes)
{
	TArray<TSharedRef<IDetailTreeNode>> Children;
	
	ParentTreeNode->GetChildren(Children);
	
	for (const TSharedRef<IDetailTreeNode> Child : Children)
	{
		GatherChildDetailTreeNodesRecursive(Child, PendingTreeNodes);
	}

	PendingTreeNodes.Append(Children);
}
