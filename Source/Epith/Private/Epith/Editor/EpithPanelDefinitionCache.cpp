#include "EpithPanelDefinitionCache.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Epith/EpithPanelDefinition.h"
#include "UObject/UObjectHash.h"

namespace EpithPanelDefinitionCache
{
	namespace
	{
		TArray<TWeakObjectPtr<UEpithPanelDefinition>> CachedDefinitions;
		
		bool bDirty = true;
		
		uint32 Version = 0;
		
		FDelegateHandle AddedHandle;
		FDelegateHandle RemovedHandle;
		FDelegateHandle RenamedHandle;
		FDelegateHandle FilesLoadedHandle;
		FDelegateHandle PropertyChangedHandle;
		
		void MarkDirty()
		{
			bDirty = true;
			++Version;
		}
		
		bool IsPanelDefinitionAsset(const FAssetData& AssetData)
		{
			return AssetData.AssetClassPath == UEpithPanelDefinition::StaticClass()->GetClassPathName();
		}
		
		void OnAssetChanged(const FAssetData& AssetData)
		{
			if (IsPanelDefinitionAsset(AssetData))
			{
				MarkDirty();
			}
		}
		
		void OnAssetRenamed(const FAssetData& AssetData, const FString& OldObjectPath)
		{
			OnAssetChanged(AssetData);
		}
		
		void OnObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event)
		{
			// A definition's target class or layout changed (including edits made in the panel designer)
			if (Cast<UEpithPanelDefinition>(Object))
			{
				++Version;
			}
		}
		
		void Rebuild()
		{
			CachedDefinitions.Reset();
			
			IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
			
			FARFilter Filter;
			Filter.ClassPaths.Add(UEpithPanelDefinition::StaticClass()->GetClassPathName());
			Filter.bRecursiveClasses = true;
			
			TArray<FAssetData> Assets;
			AssetRegistry.GetAssets(Filter, Assets);
			
			for (const FAssetData& AssetData : Assets)
			{
				if (UEpithPanelDefinition* Definition = Cast<UEpithPanelDefinition>(AssetData.GetAsset()))
				{
					CachedDefinitions.Add(Definition);
				}
			}
			
			bDirty = false;
			++Version;
		}
	}
	
	void Startup()
	{
		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		
		AddedHandle = AssetRegistry.OnAssetAdded().AddStatic(&OnAssetChanged);
		RemovedHandle = AssetRegistry.OnAssetRemoved().AddStatic(&OnAssetChanged);
		RenamedHandle = AssetRegistry.OnAssetRenamed().AddStatic(&OnAssetRenamed);
		FilesLoadedHandle = AssetRegistry.OnFilesLoaded().AddStatic(&MarkDirty);
		PropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddStatic(&OnObjectPropertyChanged);
		
		MarkDirty();
	}
	
	void Shutdown()
	{
		if (FAssetRegistryModule* AssetRegistryModule = FModuleManager::GetModulePtr<FAssetRegistryModule>("AssetRegistry"))
		{
			IAssetRegistry& AssetRegistry = AssetRegistryModule->Get();
			
			AssetRegistry.OnAssetAdded().Remove(AddedHandle);
			AssetRegistry.OnAssetRemoved().Remove(RemovedHandle);
			AssetRegistry.OnAssetRenamed().Remove(RenamedHandle);
			AssetRegistry.OnFilesLoaded().Remove(FilesLoadedHandle);
		}
		
		FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(PropertyChangedHandle);
		
		CachedDefinitions.Reset();
		bDirty = true;
	}
	
	TArray<UEpithPanelDefinition*> GetDefinitions()
	{
		TArray<UEpithPanelDefinition*> Definitions;
		Definitions.Reserve(CachedDefinitions.Num());
		
		if (!bDirty)
		{
			for (const TWeakObjectPtr<UEpithPanelDefinition>& WeakDefinition : CachedDefinitions)
			{
				UEpithPanelDefinition* Definition = WeakDefinition.Get();
				
				if (!Definition)
				{
					// Unloaded by garbage collection (or deleted); look the list up again
					bDirty = true;
					break;
				}
				
				Definitions.Add(Definition);
			}
		}
		
		if (bDirty)
		{
			Rebuild();
			
			Definitions.Reset();
			
			for (const TWeakObjectPtr<UEpithPanelDefinition>& WeakDefinition : CachedDefinitions)
			{
				if (UEpithPanelDefinition* Definition = WeakDefinition.Get())
				{
					Definitions.Add(Definition);
				}
			}
		}
		
		return Definitions;
	}
	
	uint32 GetVersion()
	{
		return Version;
	}
	
	bool HasAnyPanel(const UObject* Target)
	{
		if (!IsValid(Target))
		{
			return false;
		}
		
		const TArray<UEpithPanelDefinition*> Definitions = GetDefinitions();
		
		if (Definitions.Num() == 0)
		{
			return false;
		}
		
		// Same set of objects CreateObjectPanes looks at: the target, then its subobjects (components)
		TArray<UObject*> Objects;
		GetObjectsWithOuter(Target, Objects);
		Objects.Insert(const_cast<UObject*>(Target), 0);
		
		for (const UObject* Object : Objects)
		{
			if (!IsValid(Object))
			{
				continue;
			}
			
			for (const UEpithPanelDefinition* Definition : Definitions)
			{
				const UClass* TargetClass = Definition->GetTargetClass();
				
				if (TargetClass && Definition->HasRoot() && Object->GetClass()->IsChildOf(TargetClass))
				{
					return true;
				}
			}
		}
		
		return false;
	}
}
