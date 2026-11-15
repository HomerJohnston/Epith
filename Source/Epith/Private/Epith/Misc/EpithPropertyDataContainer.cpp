#include "Epith/Misc/EpithPropertyDataContainer.h"

#include "IPropertyRowGenerator.h"
#include "Epith/EpithLog.h"

FEpithPropertyDataContainer::FEpithPropertyDataContainer(UObject* InTarget, TSharedPtr<TSet<FName>> InExpandedPaths)
	: ExpandedPaths(InExpandedPaths)
{
	if (!IsValid(InTarget))
	{
		return;
	}
	
	Target = InTarget;
	
	Build(Target.Pin());
}

FEpithPropertyDataContainer::~FEpithPropertyDataContainer()
{
	if (PropertyRowGenerator.IsValid())
	{
		PropertyRowGenerator->OnRowsRefreshed().RemoveAll(this);
	}
}

void FEpithPropertyDataContainer::HandleRowsRefreshed()
{
	// A freshly created generator refreshes once on its first tick without changing anything, and edit conditions refresh
	// without replacing rows. Only a changed set of top-level rows means our drawn widgets are stale.
	const TArray<TSharedRef<IDetailTreeNode>>& RootNodes = PropertyRowGenerator->GetRootTreeNodes();
	
	bool bRootsChanged = RootNodes.Num() != CachedRootNodes.Num();
	
	for (int32 i = 0; !bRootsChanged && i < RootNodes.Num(); ++i)
	{
		bRootsChanged = CachedRootNodes[i].Pin() != RootNodes[i];
	}
	
	if (bRootsChanged)
	{
		OnStructureChanged.Broadcast();
	}
	else
	{
		OnRowsRefreshed.Broadcast();
	}
}

void FEpithPropertyDataContainer::Build(TStrongObjectPtr<UObject> BuildTarget)
{
	PropertyRowGenerator.Reset();
	
	static const FName PropertyEditorName = "PropertyEditor";

	FPropertyEditorModule& PropertyEditor = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>(PropertyEditorName);
	PropertyRowGenerator = PropertyEditor.CreatePropertyRowGenerator(FPropertyRowGeneratorArgs());
	PropertyRowGenerator->SetObjects( { BuildTarget.Get() } );
	PropertyRowGenerator->OnRowsRefreshed().AddRaw(this, &FEpithPropertyDataContainer::HandleRowsRefreshed);
	
	RegatherRows();
}

void FEpithPropertyDataContainer::RegatherRows()
{
	AllTreeNodes.Empty();
	AllPropertyHandles.Empty();
	ClaimedPaths.Empty();
	ChildClaimedPaths.Empty();
	CachedRootNodes.Empty();
	
	if (!PropertyRowGenerator.IsValid())
	{
		return;
	}

	TArray<TSharedRef<IDetailTreeNode>> PendingTreeNodes;
	
	PendingTreeNodes = PropertyRowGenerator->GetRootTreeNodes();
	
	for (const TSharedRef<IDetailTreeNode>& RootNode : PendingTreeNodes)
	{
		CachedRootNodes.Add(RootNode);
	}

	while (PendingTreeNodes.Num() > 0)
	{
		const TSharedRef<IDetailTreeNode> PoppedNode = PendingTreeNodes.Pop(EAllowShrinking::No);
		
		// Queue direct children only; they'll queue their own children when popped. (Gathering recursively here as well visited deep nodes many times.)
		// Include rows that are hidden right now (e.g. EditConditionHides): they still need to be findable, and hide themselves when drawn.
		TArray<TSharedRef<IDetailTreeNode>> Children;
		PoppedNode->GetChildren(Children, true);
		PendingTreeNodes.Append(Children);
		
		if (PoppedNode->GetNodeType() == EDetailNodeType::Item)
		{
			TSharedPtr<IPropertyHandle> PropertyHandle = PoppedNode->CreatePropertyHandle();
			
			if (!PropertyHandle)
			{
				//UE_LOG(LogEpith, Display, TEXT("Null property handle for IDetailTreeNode: %s"), *PoppedNode->GetNodeName().ToString());
				continue;
			}
			// TODO does GeneratePathToProperty ever generate invalid characters?
			FName PathToProperty = FName(PropertyHandle->GeneratePathToProperty());
			
			// UE_LOG(LogEpith, Display, TEXT("Property: %s"), *PathToProperty.ToString());
			
			AllTreeNodes.Add(PathToProperty, PoppedNode);
			AllPropertyHandles.Add(PathToProperty, PropertyHandle);
		}
	}
	
}
