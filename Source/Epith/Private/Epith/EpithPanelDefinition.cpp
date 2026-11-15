#include "Epith/EpithPanelDefinition.h"

#include "Epith/Editor/EpithPanelDefinitionEditor.h"

EAssetCommandResult UAssetDefinition_EpithPanelDefinition::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (UEpithPanelDefinition* PanelDefinition : OpenArgs.LoadObjects<UEpithPanelDefinition>())
	{
		// The asset editor subsystem keeps the editor alive once it's initialized
		TSharedRef<FEpithPanelDefinitionEditor> Editor = MakeShared<FEpithPanelDefinitionEditor>();
		Editor->Init(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, PanelDefinition);
	}
	
	return EAssetCommandResult::Handled;
}
