#pragma once

#include "Modules/ModuleManager.h"

#include "GPGEditorModuleBase.h"

class FEpithModule : public IModuleInterface, public FGPGEditorModuleBase
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
