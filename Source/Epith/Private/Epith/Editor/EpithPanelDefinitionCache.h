#pragma once

#include "CoreMinimal.h"

class UEpithPanelDefinition;

/**
 * Keeps the list of panel definition assets, so it isn't looked up (and every definition loaded) on each query.
 * The list is refreshed only when the asset registry reports a panel definition being added, removed or renamed,
 * or when a cached definition has been unloaded.
 */
namespace EpithPanelDefinitionCache
{
	/** Starts listening for asset registry and property changes. Called by the editor subsystem. */
	void Startup();
	
	void Shutdown();
	
	/** All panel definition assets (loaded on first use). */
	TArray<UEpithPanelDefinition*> GetDefinitions();
	
	/** Changes whenever the list or any definition changes, so callers can drop answers they cached. */
	uint32 GetVersion();
	
	/** True if the target or one of its subobjects (e.g. an actor's components) matches a panel definition. Cheap once the list is cached. */
	bool HasAnyPanel(const UObject* Target);
}
