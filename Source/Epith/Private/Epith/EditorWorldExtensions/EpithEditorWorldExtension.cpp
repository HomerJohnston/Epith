#include "Epith/EditorWorldExtensions/EpithEditorWorldExtension.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/DeveloperSettings/EpithProjectSettings.h"
#include "Epith/Subsystems/EpithEditorSubsystem.h"
#include "Epith/Windows/EpithViewportPopup.h"
#include "Subsystems/EditorActorSubsystem.h"

const FName EpithMetaDataKey = TEXT("Epith");

void UEpithEditorWorldExtension::Init()
{
	SetActive(true);
}

bool UEpithEditorWorldExtension::InputKey(FEditorViewportClient* InViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (Event == IE_DoubleClick)
	{
		UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
		TArray<AActor*> SelectedActors;
		
		if (!EditorActorSubsystem)
		{
			return false;
		}

		SelectedActors = EditorActorSubsystem->GetSelectedLevelActors();
		
		if (SelectedActors.Num() != 1)
		{
			return false;
		}

		AActor* Actor = SelectedActors[0];

		const UClass* Class = Actor->GetClass();
		
		// Dig to native class
		bool bFoundEpithAttributeSet = false;
		
		const UEpithProjectSettings* ProjectSettings = GetDefault<UEpithProjectSettings>();

		const TInstancedStruct<FEpithWindowElement>* PanelRoot = GetPanelRoot(Actor);
		
		if (PanelRoot)
		{
			const FEpithWindowElement& WindowRoot = PanelRoot->Get<FEpithWindowElement>();
			FEpithViewportPopup::Show(Actor, WindowRoot);
			return true;
		}
	}

	return false;
}

const TInstancedStruct<FEpithWindowElement>* UEpithEditorWorldExtension::GetPanelRoot(AActor* Actor) const
{
	const UEpithPanelDefinition* PanelDefinition = nullptr;
	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	
	FARFilter Filter;
	Filter.ClassPaths.Add(FTopLevelAssetPath(UEpithPanelDefinition::StaticClass()->GetClassPathName()));
	Filter.bRecursiveClasses = true;
	
	TArray<FAssetData> OutAssets;
	AssetRegistry.GetAssets(Filter, OutAssets);
	
	if (OutAssets.Num() == 0)
	{
		return nullptr;
	}
	
	// TODO this might get slow if you have hundreds of these? Will a game ever have hundreds of these? Can I include some other metadata in assets to search for faster with the asset registry?
	TArray<TPair<uint8, const UEpithPanelDefinition*>> FoundPanelDefinitionsForClass;
	
	// We need to iterate all of the panel definitions to find the one most suitable to our actor.
	for (const FAssetData& AssetData : OutAssets)
	{
		PanelDefinition = Cast<const UEpithPanelDefinition>(AssetData.GetAsset());	
		
		if (PanelDefinition && Actor->GetClass()->IsChildOf(PanelDefinition->GetTargetActorType()))
		{
			uint8 Indirections = 0; // I am making the humble assumption that there will never be more than 255 parent classes
			bool bMatch = false;
			
			const UClass* CandidateClass = Actor->GetClass();
			
			// Early out - if we find an asset with an exact match we don't need to keep iterating
			if (PanelDefinition->GetTargetActorType() == CandidateClass)
			{
				return PanelDefinition->GetRoot();
			}
	
			while (CandidateClass)
			{
				// We don't need to check if low level classes UObject, UObjectBaseUtility, etc. are matches
				if (!CandidateClass->IsChildOf(AActor::StaticClass()))
				{
					break;
				}
				
				// We will NOT return a perfect match result here so that we can keep looking for duplicates to inform the user 
				if (PanelDefinition->GetTargetActorType() == CandidateClass)
				{
					bMatch = true;
					break;
				}
		
				CandidateClass = CandidateClass->GetSuperClass();
				++Indirections;
			}
			
			if (bMatch)
			{
				FoundPanelDefinitionsForClass.Add( {Indirections, PanelDefinition} );
			}
		}
	}
	
	if (FoundPanelDefinitionsForClass.Num() == 0)
	{
		return nullptr;
	}
	
	FoundPanelDefinitionsForClass.Sort([] (const TPair<uint8, const UEpithPanelDefinition*>& Left, const TPair<uint8, const UEpithPanelDefinition*>& Right)
	{
		return Left.Key < Right.Key;
	});
	
	return FoundPanelDefinitionsForClass[0].Value->GetRoot();
}
