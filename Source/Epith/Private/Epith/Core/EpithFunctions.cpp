#include "Epith/Core/EpithFunctions.h"

#include "Epith/EpithLog.h"
#include "Epith/Subsystems/EpithEditorSubsystem.h"

/*
TSharedPtr<FEpithWindowElement> Epith::BeginLayout(UClass* Class)
{
	if (!Class)
	{
		UE_LOG(LogEpith, Error, TEXT("You need to set a Class!"))
		return nullptr;
	}
	
	if (!GEditor)
	{
		UE_LOG(LogEpith, Error, TEXT("Couldn't find GEditor!"))
		return nullptr;
	}
	
	UEpithEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UEpithEditorSubsystem>();
	
	return Subsystem->BeginLayout(Class);
}
*/