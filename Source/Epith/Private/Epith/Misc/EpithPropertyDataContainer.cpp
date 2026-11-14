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
}

FEpithPropertyDataContainer::~FEpithPropertyDataContainer()
{
}

void FEpithPropertyDataContainer::Build(TStrongObjectPtr<UObject> BuildTarget)
{
	// TODO this is re-running every time I change a property, can I fix that somehow?
	AllTreeNodes.Empty();
	AllPropertyHandles.Empty();
	ClaimedPaths.Empty();
	PropertyRowGenerator.Reset();
	
	static const FName PropertyEditorName = "PropertyEditor";

	FPropertyEditorModule& PropertyEditor = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>(PropertyEditorName);
	PropertyRowGenerator = PropertyEditor.CreatePropertyRowGenerator(FPropertyRowGeneratorArgs());
	PropertyRowGenerator->SetObjects( { BuildTarget.Get() } );

	TArray<TSharedRef<IDetailTreeNode>> PendingTreeNodes;
	
	PendingTreeNodes = PropertyRowGenerator->GetRootTreeNodes();

	while (PendingTreeNodes.Num() > 0)
	{
		const TSharedRef<IDetailTreeNode> PoppedNode = PendingTreeNodes.Pop(EAllowShrinking::No);
		
		// Queue direct children only; they'll queue their own children when popped. (Gathering recursively here as well visited deep nodes many times.)
		TArray<TSharedRef<IDetailTreeNode>> Children;
		PoppedNode->GetChildren(Children);
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
