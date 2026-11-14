#pragma once

#include "StructUtils/InstancedStruct.h"

#include "EpithEditorSubsystem.generated.h"

class UEpithEditorWorldExtension;
struct FEpithWindowElement;

UCLASS()
class UEpithEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

	void Initialize(FSubsystemCollectionBase& Collection) override;
	
	void Deinitialize() override;
	
	void OnPreBeginPIE(bool bIsSimulating);

	void OnPostWorldInitialization(UWorld* World, const UWorld::InitializationValues InitValues);
	
	UPROPERTY()
	TObjectPtr<UEpithEditorWorldExtension> WorldExtension;
};