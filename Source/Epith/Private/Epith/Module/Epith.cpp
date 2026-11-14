// Copyright Epic Games, Inc. All Rights Reserved.

#include "Epith/Module/Epith.h"
#include "EpithModule.h"

#include "AssetTypeCategories.h"
#include "Epith/Customizations/PropertyTypeCustomization_EpithPropertySelector.h"
#include "Epith/Misc/EpithPropertySelector.h"
#include "Epith/Style/EpithStyle.h"

#define LOCTEXT_NAMESPACE "FEpithModule"

FAssetCategoryPath FEpithAssetCategoryPaths::Epith(LOCTEXT("Epith", "Epith"));

void FEpithModule::StartupModule()
{
	REGISTER_PROPERTY_CUSTOMIZATION(FEpithPropertySelector, FPropertyTypeCustomization_EpithPropertySelector);
	
	StartupModuleBase();
	
	FEpithStyle::Initialize();
}

void FEpithModule::ShutdownModule()
{
	FEpithStyle::Shutdown();
	
	ShutdownModuleBase();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FEpithModule, Epith)