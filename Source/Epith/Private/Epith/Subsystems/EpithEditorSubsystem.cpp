#include "Epith/Subsystems/EpithEditorSubsystem.h"

#include "EditorWorldExtension.h"
#include "LevelEditor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Epith/EpithLog.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/EditorWorldExtensions/EpithEditorWorldExtension.h"
#include "Epith/Widgets/SEpithViewportMasterPane.h"

void UEpithEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	WorldExtension = NewObject<UEpithEditorWorldExtension>();
	
	FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &ThisClass::OnPostWorldInitialization);
	FEditorDelegates::PreBeginPIE.AddUObject(this, &ThisClass::OnPreBeginPIE);
}

void UEpithEditorSubsystem::Deinitialize()
{
	FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);
	FEditorDelegates::PreBeginPIE.RemoveAll(this);
	
	SEpithViewportMasterPane::CloseAll();
	
	Super::Deinitialize();
}

void UEpithEditorSubsystem::OnPreBeginPIE(bool bIsSimulating)
{
	// Panels edit editor-world objects; don't leave them floating over a play session.
	SEpithViewportMasterPane::CloseAll();
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
