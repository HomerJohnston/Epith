#pragma once

#include "Toolkits/AssetEditorToolkit.h"

class IDetailsView;
class SEpithElementDetails;
class SEpithPanelDesigner;
class UEpithPanelDefinition;

/**
 * The asset editor for Epith panel definitions: the panel designer, whose side panes hold the definition's settings (PANEL
 * PROPERTIES) and the selected element's (ELEMENT).
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
	TSharedRef<SDockTab> SpawnDesignerTab(const FSpawnTabArgs& Args);

	TWeakObjectPtr<UEpithPanelDefinition> Definition;

	/** The definition's settings, minus Root: the designer's PANEL PROPERTIES section. */
	TSharedPtr<IDetailsView> DetailsView;

	TSharedPtr<SEpithPanelDesigner> Designer;

	/** Settings of the element selected in the designer, shown in the designer's side pane. */
	TSharedPtr<SEpithElementDetails> ElementDetails;

	static const FName DesignerTabId;
};
