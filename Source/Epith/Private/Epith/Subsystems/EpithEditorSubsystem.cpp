#include "Epith/Subsystems/EpithEditorSubsystem.h"

#include "EditorWorldExtension.h"
#include "LevelEditor.h"
#include "Epith/EpithLog.h"
#include "Epith/EditorWorldExtensions/EpithEditorWorldExtension.h"
#include "Epith/Interfaces/IEpithActorInterface.h"

void UEpithEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	WorldExtension = NewObject<UEpithEditorWorldExtension>();
	
	FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &ThisClass::OnPostWorldInitialization);
	FEditorDelegates::OnEditorInitialized.AddUObject(this, &ThisClass::OnEditorInitialized);
}

void UEpithEditorSubsystem::OnEditorInitialized(double StartupTime)
{
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		
		UE_LOG(LogEpith, Display, TEXT("%s"), *Class->GetName());
		
		if (IEpithActorInterface* EpithInterface = Cast<IEpithActorInterface>(Class->GetDefaultObject<UObject>()))
		{
			EpithInterface->EpithBootstrapFunction();
		}
	}
}

void UEpithEditorSubsystem::OnPostWorldInitialization(UWorld* World, const UWorld::InitializationValues InitValues)
{
	UEditorWorldExtensionManager* ExtensionManager = GEditor->GetEditorWorldExtensionsManager();

	if (!ExtensionManager)
	{
		return;
	}
	
	UEditorWorldExtensionCollection* ExtensionCollection = ExtensionManager->GetEditorWorldExtensions(World);

	ExtensionCollection->AddExtension(WorldExtension);
}

void UEpithEditorSubsystem::SetAttributeMetaData(const UClass* Class, FName PropertyName, FName EpithAttribute, FString AttributeMetaData)
{
	FEpithClassDefinition& ClassDefinition = Definitions.ClassMetaDatas.FindOrAdd(Class);
	
	FEpithPropertyDefinition& PropertyDefinition = ClassDefinition.PropertyMetaDatas.FindOrAdd(PropertyName);
	
	if (PropertyDefinition.EpithAttribute != NAME_None)
	{
		UE_LOG(LogEpith, Warning, TEXT("Overwriting existing property attribute! UClass: %s, PropertyName: %s, Attribute: %s, Old MetaData: %s, New MetaData: %s"), 
			*Class->GetName(), *PropertyName.ToString(), *EpithAttribute.ToString(), *PropertyDefinition.AttributeMetaData, *AttributeMetaData);
	}
	
	PropertyDefinition.EpithAttribute = EpithAttribute;
	PropertyDefinition.AttributeMetaData = AttributeMetaData;
}

FEpithClassDefinition* UEpithEditorSubsystem::GetClassAttributeSet(const UClass* Class)
{
	return Definitions.ClassMetaDatas.Find(Class);
}
