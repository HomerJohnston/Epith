// Copyright Epic Games, Inc. All Rights Reserved.

#include "Epith/Module/Epith.h"
#include "EpithModule.h"

#include "AssetTypeCategories.h"
#include "Epith/Customizations/PropertyTypeCustomization_EpithPropertySelector.h"
#include "Epith/Misc/EpithPropertySelector.h"
#include "Epith/Style/EpithStyle.h"
#include "UObject/CoreRedirects.h"

#define LOCTEXT_NAMESPACE "FEpithModule"

FAssetCategoryPath FEpithAssetCategoryPaths::Epith(LOCTEXT("Epith", "Epith"));

void FEpithModule::StartupModule()
{
	// Renamed types. Saved panel definitions still refer to the old names; this lets them load.
	TArray<FCoreRedirect> Redirects;
	Redirects.Emplace(ECoreRedirectFlags::Type_Struct, TEXT("/Script/Epith.EpithWindowElement_FoldoutGroup"), TEXT("/Script/Epith.EpithWindowElement_Foldout"));
	FCoreRedirects::AddRedirectList(Redirects, TEXT("Epith"));
	
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