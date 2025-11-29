#include "Epith/Core/EpithFunctions.h"

#include "Epith/EpithLog.h"
#include "Epith/Subsystems/EpithEditorSubsystem.h"

Epith::ScopeContext::ScopeContext(const UClass* InClass)
{
	Epith::SetOperatingClass(InClass);
}

Epith::ScopeContext::~ScopeContext()
{
	Epith::SetOperatingClass(nullptr);
}

const TArray<FName> Epith::EpithAttributeNames
{
	TEXT("TabGroup"),
	TEXT("BoxGroup"),
	TEXT("FoldoutGroup")
};

const UClass* Epith::__OperatingClass;

void Epith::SetOperatingClass(const UClass* Class)
{
	__OperatingClass = Class;
}

void Epith::Set(EEpithAttribute Attribute, FString AttributeMetaData, FName Property)
{
	if (!__OperatingClass)
	{
		UE_LOG(LogEpith, Error, TEXT("You need to set the Epith::CurrentClass"))
		return;
	}
	
	if (!GEditor)
	{
		// TODO log error
		return;
	}
	
	UEpithEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UEpithEditorSubsystem>();
	
	Subsystem->SetAttributeMetaData(__OperatingClass, Property, GetAttributeFName(Attribute), AttributeMetaData);
}
