#include "Epith/Settings/EpithEditorSettings.h"

#include "Epith/Widgets/SEpithViewportMasterPane.h"

UEpithEditorSettings::UEpithEditorSettings()
{
	// Editor Preferences, not Project Settings
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("Epith");
}

FName UEpithEditorSettings::GetContainerName() const
{
	return TEXT("Editor");
}

#if WITH_EDITOR
void UEpithEditorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	// Turning Epith off also closes any panel that's open
	if (bDisableEpith)
	{
		SEpithViewportMasterPane::CloseAll();
	}
}
#endif
