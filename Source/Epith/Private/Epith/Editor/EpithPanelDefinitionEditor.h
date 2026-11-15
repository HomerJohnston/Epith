#pragma once

#include "Toolkits/AssetEditorToolkit.h"

class IDetailsView;
class SEpithPanelDesigner;
class UEpithPanelDefinition;

/**
 * The asset editor for Epith panel definitions: the usual Details view, with the panel designer beside it.
 * Opened by UAssetDefinition_EpithPanelDefinition::OpenAssets.
 */
class FEpithPanelDefinitionEditor : public FAssetEditorToolkit
{
public:
	void Init(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UEpithPanelDefinition* InDefinition);

	// FAssetEditorToolkit
	FName GetToolkitFName() const override;
	FText GetBaseToolkitName() const override;
	FString GetWorldCentricTabPrefix() const override;
	FLinearColor GetWorldCentricTabColorScale() const override;
	void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

private:
	TSharedRef<SDockTab> SpawnDetailsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnDesignerTab(const FSpawnTabArgs& Args);

	TWeakObjectPtr<UEpithPanelDefinition> Definition;

	TSharedPtr<IDetailsView> DetailsView;

	TSharedPtr<SEpithPanelDesigner> Designer;

	static const FName DetailsTabId;
	static const FName DesignerTabId;
};
