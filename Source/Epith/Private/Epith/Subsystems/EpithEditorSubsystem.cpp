#include "Epith/Subsystems/EpithEditorSubsystem.h"

#include "EditorWorldExtension.h"
#include "LevelEditor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Epith/EpithLog.h"
#include "Epith/EpithPanelDefinition.h"
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
