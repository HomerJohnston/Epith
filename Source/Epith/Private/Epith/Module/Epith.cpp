// Copyright Epic Games, Inc. All Rights Reserved.

#include "Epith/Module/Epith.h"

#include "AssetTypeCategories.h"
#include "Epith/Customizations/PropertyTypeCustomization_EpithPropertySelector.h"
#include "Epith/Misc/EpithPropertySelector.h"

#define LOCTEXT_NAMESPACE "FEpithModule"

EAssetTypeCategories::Type FEpithModule::EpithAssetCategory = static_cast<EAssetTypeCategories::Type>(0);
FAssetCategoryPath FEpithAssetCategoryPaths::Epith(LOCTEXT("Epith", "Epith"));

void FEpithModule::StartupModule()
{
	REGISTER_PROPERTY_CUSTOMIZATION(FEpithPropertySelector, FPropertyTypeCustomization_EpithPropertySelector);
	
	StartupModuleBase();
}

void FEpithModule::ShutdownModule()
{
	ShutdownModuleBase();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FEpithModule, Epith)