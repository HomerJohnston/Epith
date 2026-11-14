#pragma once

#include "IDetailTreeNode.h"
#include "PropertyHandle.h"
#include "Epith/EpithLog.h"

struct FEpithPropertyDataContainer
{
public:
	FEpithPropertyDataContainer(UObject* InTarget);

	~FEpithPropertyDataContainer();
	
protected:
	TWeakObjectPtr<UObject> Target;

	TSharedPtr<class IPropertyRowGenerator> PropertyRowGenerator;
	
	TMap<FName, TSharedPtr<IDetailTreeNode>> AllTreeNodes;
	TMap<FName, TSharedPtr<IPropertyHandle>> AllPropertyHandles;
	
	/** Paths whose widgets have been handed out. A row's widgets can only be shown once per container. */
	TSet<FName> ClaimedPaths;

	void Build(TStrongObjectPtr<UObject> BuildTarget);

public:
	/**
	 * Returns the property that stops this handle from being edited on placed instances, or null if it's instance-editable.
	 * That's any property in the chain (the property itself or a struct/container it lives in) flagged CPF_DisableEditOnInstance:
	 * EditDefaultsOnly in C++, or a Blueprint variable that isn't Instance Editable.
	 */
	static const FProperty* FindInstanceEditBlocker(TSharedPtr<IPropertyHandle> PropertyHandle)
	{
		for (TSharedPtr<IPropertyHandle> Current = PropertyHandle; Current.IsValid(); Current = Current->GetParentHandle())
		{
			const FProperty* Property = Current->GetProperty();
			
			if (Property && Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance))
			{
				return Property;
			}
		}
		
		return nullptr;
	}
	
	/** Returns true the first time a path is claimed, false afterwards. Use one container per drawn layout. */
	bool TryClaimPath(FName PropertyPath)
	{
		bool bAlreadyClaimed = false;
		ClaimedPaths.Add(PropertyPath, &bAlreadyClaimed);
		return !bAlreadyClaimed;
	}
	
	TSharedPtr<IPropertyHandle> FindPropertyHandle(FName PropertyPath)
	{
		TSharedPtr<IPropertyHandle>* Found = AllPropertyHandles.Find(PropertyPath);
		
		return (Found) ? *Found : nullptr;
	}
	
	/** True if the target is a placed instance rather than a class default object or archetype. */
	bool IsEditingInstance() const
	{
		return Target.IsValid() && !Target->IsTemplate();
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
