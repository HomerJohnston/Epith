#pragma once

#include "IDetailTreeNode.h"
#include "PropertyHandle.h"

struct FEpithPropertyDataContainer
{
public:
	FEpithPropertyDataContainer(UObject* Target);

protected:
	TSharedPtr<class IPropertyRowGenerator> PropertyRowGenerator;
	
	TMap<FName, TSharedPtr<IDetailTreeNode>> AllTreeNodes;
	TMap<FName, TSharedPtr<IPropertyHandle>> AllPropertyHandles;
	
	void Build(UObject* Actor);
	
	void GatherChildDetailTreeNodesRecursive(TSharedRef<IDetailTreeNode> ParentTreeNode, TArray<TSharedRef<IDetailTreeNode>>& PendingTreeNodes);
	
public:
	TSharedPtr<IPropertyHandle> FindPropertyHandle(FName PropertyPath)
	{
		TSharedPtr<IPropertyHandle>* Found = AllPropertyHandles.Find(PropertyPath);
		
		return (Found) ? *Found : nullptr;
	}
	
	TSharedPtr<IDetailTreeNode> FindTreeNode(FName PropertyPath)
	{
		TSharedPtr<IDetailTreeNode>* Found = AllTreeNodes.Find(PropertyPath);
		
		return (Found) ? *Found : nullptr;
	}
	
	const TMap<FName, TSharedPtr<IPropertyHandle>> GetPropertyHandles() const
	{
		return AllPropertyHandles;
	}
	
	TArray<FName> GetPropertyPaths() const
	{
		TArray<FName> AllKeys;
		AllPropertyHandles.GetKeys(AllKeys);
		
		return AllKeys;
	}
};