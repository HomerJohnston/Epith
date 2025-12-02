#include "Epith/Misc/EpithPropertyDataContainer.h"

#include "IPropertyRowGenerator.h"
#include "Epith/EpithLog.h"

FEpithPropertyDataContainer::FEpithPropertyDataContainer(UObject* InTarget)
{
	if (!IsValid(InTarget))
	{
		return;
	}
	
	Target = InTarget;
	
	Build(Target.Pin());
	
	//FCoreUObjectDelegates::OnObjectsReplaced.AddRaw(this, &FEpithPropertyDataContainer::OnObjectsReinstanced);
}

FEpithPropertyDataContainer::~FEpithPropertyDataContainer()
{
	//FCoreUObjectDelegates::OnObjectsReplaced.RemoveAll(this);
}

void FEpithPropertyDataContainer::Build(TStrongObjectPtr<UObject> BuildTarget)
{
	static const FName PropertyEditorName = "PropertyEditor";

	FPropertyEditorModule& PropertyEditor = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>(PropertyEditorName);
	PropertyRowGenerator = PropertyEditor.CreatePropertyRowGenerator(FPropertyRowGeneratorArgs());
	PropertyRowGenerator->SetObjects( { BuildTarget.Get() } );

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

void FEpithPropertyDataContainer::OnObjectsReinstanced(const TMap<UObject*, UObject*>& ReplacementMap)
{
	UObject* const* NewTarget = ReplacementMap.Find(Target.Get());
	
	if (NewTarget)
	{
		Target = *NewTarget;
		Build(Target.Pin());
	}
}
