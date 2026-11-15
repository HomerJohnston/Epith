#pragma once

#include "Containers/Ticker.h"
#include "StructUtils/InstancedStruct.h"

#include "EpithEditorSubsystem.generated.h"

class FEpithHoverHint;
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
	
	/** The "Double-click to edit" hint over actors that have a panel. */
	TSharedPtr<FEpithHoverHint> HoverHint;
	
	FTSTicker::FDelegateHandle HoverHintTickHandle;
};