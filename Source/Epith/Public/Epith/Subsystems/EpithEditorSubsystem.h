#pragma once

#include "EpithEditorSubsystem.generated.h"

class UEpithEditorWorldExtension;

struct FEpithPropertyDefinition
{
	FName EpithAttribute;
	FString AttributeMetaData;
};

struct FEpithClassDefinition
{
	TMap<FName, FEpithPropertyDefinition> PropertyMetaDatas;
};

struct FEpithProgramDefinition
{
	TMap<const UClass*, FEpithClassDefinition> ClassMetaDatas;
};

UCLASS()
class UEpithEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

	void Initialize(FSubsystemCollectionBase& Collection) override;

	void OnEditorInitialized(double StartupTime);
	
	void OnPostWorldInitialization(UWorld* World, const UWorld::InitializationValues InitValues);
	
	UPROPERTY()
	TObjectPtr<UEpithEditorWorldExtension> WorldExtension;
	
	FEpithProgramDefinition Definitions;
	
public:
	void SetAttributeMetaData(const UClass* Class, FName PropertyName, FName EpithAttribute, FString AttributeMetaData);
	
	FEpithClassDefinition* GetClassAttributeSet(const UClass* Class);
};