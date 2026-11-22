#pragma once

#include "IDetailTreeNode.h"
#include "PropertyHandle.h"
#include "Epith/EpithLog.h"

struct FEpithPropertyDataContainer
{
public:
	/**
	 * @param InExpandedPaths Which rows are expanded. Pass the same set to each rebuilt container so expansion survives rebuilds.
	 * @param bInReadOnly The rows only show values: nothing drawn from this container can change the target (see IsReadOnly).
	 */
	FEpithPropertyDataContainer(UObject* InTarget, TSharedPtr<TSet<FName>> InExpandedPaths = nullptr, bool bInReadOnly = false);

	~FEpithPropertyDataContainer();

	/** The object whose properties this container shows. */
	UObject* GetTarget() const { return Target.Get(); }

	/**
	 * True for a look-only panel (the designer's REAL VIEW, which shows the class defaults). Everything that could change the
	 * target is disabled: values, reset arrows, edit-condition toggles and special controls. Rows can still be expanded.
	 */
	bool IsReadOnly() const { return bReadOnly; }

protected:
	TWeakObjectPtr<UObject> Target;

	bool bReadOnly = false;

	TSharedPtr<class IPropertyRowGenerator> PropertyRowGenerator;

	TMap<FName, TSharedPtr<IDetailTreeNode>> AllTreeNodes;
	TMap<FName, TSharedPtr<IPropertyHandle>> AllPropertyHandles;

	/** Paths whose widgets have been handed out. A row's widgets can only be shown once per container. */
	TSet<FName> ClaimedPaths;

	/** Like ClaimedPaths, but for rows drawn as children of an expanded struct/array. These are released when that row redraws its children. */
	TSet<FName> ChildClaimedPaths;

	/** The generator's top-level rows when we built. If a refresh replaces them, everything drawn from us is stale. */
	TArray<TWeakPtr<IDetailTreeNode>> CachedRootNodes;

	TSharedPtr<TSet<FName>> ExpandedPaths;

	void HandleRowsRefreshed();

	/** The target was replaced (a Blueprint recompile, or construction scripts recreating a component): follow it right away. */
	void HandleObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap);

	void Build(TStrongObjectPtr<UObject> BuildTarget);

public:
	/**
	 * Re-reads the rows from the existing row generator (after OnStructureChanged) and clears all claims, ready for a full redraw.
	 * Reusing the generator matters: a new generator refreshes once on its first tick, which would start the cycle over.
	 */
	void RegatherRows();

protected:

public:
	/**
	 * Broadcast when the row generator refreshed its rows (an array changed size, a row was shown/hidden, ...) but kept its top-level rows.
	 * Expanded rows use this to check whether their own children changed and redraw just those. Fires during the generator's tick, not during paint.
	 */
	FSimpleMulticastDelegate OnRowsRefreshed;

	/**
	 * Broadcast when a refresh replaced the generator's top-level rows (rare: e.g. an instanced sub-object was swapped).
	 * Everything drawn from this container is stale, so the owner should rebuild with a new container.
	 */
	FSimpleMulticastDelegate OnStructureChanged;

	bool IsExpanded(FName Path) const
	{
		return ExpandedPaths.IsValid() && ExpandedPaths->Contains(Path);
	}

	void SetExpanded(FName Path, bool bExpanded)
	{
		if (!ExpandedPaths.IsValid())
		{
			ExpandedPaths = MakeShared<TSet<FName>>();
		}

		if (bExpanded)
		{
			ExpandedPaths->Add(Path);
		}
		else
		{
			ExpandedPaths->Remove(Path);
		}
	}

	/**
	 * Whether it's safe to ask the handle if it differs from its default (DiffersFromDefault, CanResetToDefault).
	 *
	 * The default comes from the object's archetype: for a placed object its class defaults, but for a class default object the
	 * PARENT class's defaults, which don't have a property the class itself adds. The engine checks for that on ordinary
	 * properties, but not on the members of an instanced struct, where it reads the missing property out of the parent's memory
	 * anyway and crashes. So: false if any object being shown has an archetype without the handle's outermost property (there
	 * is no default to compare with, so nothing differs and nothing can be reset).
	 */
	static bool HasDefaultToCompare(TSharedPtr<IPropertyHandle> PropertyHandle)
	{
		if (!PropertyHandle.IsValid() || !PropertyHandle->IsValidHandle())
		{
			return false;
		}

		// The outermost property in the chain: the one on the object itself
		const FProperty* TopProperty = nullptr;

		for (TSharedPtr<IPropertyHandle> Current = PropertyHandle; Current.IsValid(); Current = Current->GetParentHandle())
		{
			if (const FProperty* Property = Current->GetProperty())
			{
				TopProperty = Property;
			}
		}

		const UClass* OwnerClass = TopProperty ? TopProperty->GetOwnerClass() : nullptr;

		// Not a property of an object's class (a standalone struct, say): nothing for us to check
		if (!OwnerClass)
		{
			return true;
		}

		TArray<UObject*> OuterObjects;
		PropertyHandle->GetOuterObjects(OuterObjects);

		for (const UObject* Object : OuterObjects)
		{
			const UObject* Archetype = Object ? Object->GetArchetype() : nullptr;

			if (!Archetype || !Archetype->IsA(OwnerClass))
			{
				return false;
			}
		}

		return true;
	}

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

	/**
	 * True for a path to an element of an array, set or map, or to anything inside one (e.g. "Numbers.Numbers[0]",
	 * "Items.Items[2].Value"). Elements come and go as the container changes size, so they can't be picked on their own;
	 * a panel shows the whole container instead.
	 */
	static bool IsContainerElementPath(FName PropertyPath)
	{
		int32 BracketIndex = INDEX_NONE;
		return PropertyPath.ToString().FindChar(TEXT('['), BracketIndex);
	}

	/** Returns true the first time a path is claimed, false afterwards. Use one container per drawn layout. */
	bool TryClaimPath(FName PropertyPath)
	{
		if (ClaimedPaths.Contains(PropertyPath) || ChildClaimedPaths.Contains(PropertyPath))
		{
			return false;
		}

		ClaimedPaths.Add(PropertyPath);
		return true;
	}

	/** Same as TryClaimPath, for rows drawn as children of an expanded row. */
	bool TryClaimChildPath(FName PropertyPath)
	{
		if (ClaimedPaths.Contains(PropertyPath) || ChildClaimedPaths.Contains(PropertyPath))
		{
			return false;
		}

		ChildClaimedPaths.Add(PropertyPath);
		return true;
	}

	/** Releases child claims nested under ParentPath (e.g. "MyArray[0]", "MyStruct.Value"), before that row redraws its children. */
	void ReleaseChildClaimsUnder(FName ParentPath)
	{
		const FString Parent = ParentPath.ToString();

		for (auto It = ChildClaimedPaths.CreateIterator(); It; ++It)
		{
			const FString Path = It->ToString();

			if (Path.Len() > Parent.Len() && Path.StartsWith(Parent, ESearchCase::CaseSensitive) && (Path[Parent.Len()] == TEXT('.') || Path[Parent.Len()] == TEXT('[')))
			{
				It.RemoveCurrent();
			}
		}
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
