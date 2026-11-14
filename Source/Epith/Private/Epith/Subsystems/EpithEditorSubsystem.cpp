#include "Epith/Subsystems/EpithEditorSubsystem.h"

#include "EditorWorldExtension.h"
#include "LevelEditor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Epith/EpithLog.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/EditorWorldExtensions/EpithEditorWorldExtension.h"

void UEpithEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	WorldExtension = NewObject<UEpithEditorWorldExtension>();
	
	FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &ThisClass::OnPostWorldInitialization);
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
