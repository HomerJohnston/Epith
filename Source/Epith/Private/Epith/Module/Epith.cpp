// Copyright Epic Games, Inc. All Rights Reserved.

#include "Epith/Module/Epith.h"

#include "AssetTypeCategories.h"

#define LOCTEXT_NAMESPACE "FEpithModule"

EAssetTypeCategories::Type FEpithModule::EpithAssetCategory = static_cast<EAssetTypeCategories::Type>(0);
FAssetCategoryPath FEpithAssetCategoryPaths::Epith(LOCTEXT("Epith", "Epith"));

void FEpithModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FEpithModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FEpithModule, Epith)