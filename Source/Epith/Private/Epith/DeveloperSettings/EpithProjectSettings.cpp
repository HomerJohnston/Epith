#include "Epith/DeveloperSettings/EpithProjectSettings.h"

const TInstancedStruct<FEpithWindowElement>* UEpithProjectSettings::GetPanelRoot(AActor* Actor) const
{
	// We need to walk up the class inheritance tree of our actor. The map might only have a parent class, not the actual class of the actor.
	
	const UClass* CandidateClass = Actor->GetClass();
	
	const TInstancedStruct<FEpithWindowElement>* FoundRoot = nullptr;
	
	while (CandidateClass && !FoundRoot)
	{
		FoundRoot = PanelDefinitions.Find(CandidateClass);
		
		if (!FoundRoot)
		{
			CandidateClass = CandidateClass->GetSuperClass();
		}
	}
	
	return FoundRoot;
}
