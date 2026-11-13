#pragma once

#include "IDetailTreeNode.h"
#include "PropertyHandle.h"
#include "Epith/EpithLog.h"

struct FEpithPropertyDataContainer
{
public:
	FEpithPropertyDataContainer(UObject* InTarget);

	~FEpithPropertyDataContainer();
	
	void OnObjectsReinstanced(const TMap<UObject*, UObject*>& ReplacementMap);
	
protected:
	TWeakObjectPtr<UObject> Target;

	TSharedPtr<class IPropertyRowGenerator> PropertyRowGenerator;
	
	TMap<FName, TSharedPtr<IDetailTreeNode>> AllTreeNodes;
	TMap<FName, TSharedPtr<IPropertyHandle>> AllPropertyHandles;

	void Build(TStrongObjectPtr<UObject> BuildTarget);
	
	void GatherChildDetailTreeNodesRecursive(TSharedRef<IDetailTreeNode> ParentTreeNode, TArray<TSharedRef<IDetailTreeNode>>& PendingTreeNodes);

public:
	void RefreshPropertyHandles();
	
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
