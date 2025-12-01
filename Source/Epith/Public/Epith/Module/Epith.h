// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetTypeCategories.h"
#include "Modules/ModuleManager.h"

#include "Epith/Private/GPGEditorModuleBase.h"

struct FEpithAssetCategoryPaths : EAssetCategoryPaths
{
	static FAssetCategoryPath Epith;
};

class FEpithModule : public IModuleInterface, public FGPGEditorModuleBase
{
public:

	static EAssetTypeCategories::Type EpithAssetCategory;
	
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
