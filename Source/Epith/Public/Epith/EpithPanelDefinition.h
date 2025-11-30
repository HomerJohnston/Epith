#pragma once

#include "AssetDefinitionDefault.h"
#include "Module/Epith.h"
#include "Widgets/EpithWindowElement.h"

#include "EpithPanelDefinition.generated.h" 

UCLASS(Blueprintable)
class EPITH_API UEpithPanelDefinition : public UObject
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Epith")
	TSubclassOf<AActor> TargetActorType;
	
	UPROPERTY(EditDefaultsOnly, Category = "Epith", meta = (ShowOnlyInnerProperties, ExcludeBaseStruct))
	TInstancedStruct<FEpithWindowElement> Root;
	
public:
	TSubclassOf<AActor> GetTargetActorType() const
	{
		return TargetActorType;
	}
	
	const TInstancedStruct<FEpithWindowElement>* GetRoot() const
	{
		if (Root.IsValid())
		{
			return &Root;
		}
		
		return nullptr;
	}
};

UCLASS()
class EPITH_API UAssetDefinition_EpithPanelDefinition : public UAssetDefinitionDefault
{
	GENERATED_BODY()
	
public:
	TSoftClassPtr<UObject> GetAssetClass() const override
	{
		return UEpithPanelDefinition::StaticClass();
	}
	
	FLinearColor GetAssetColor() const override
	{
		return FLinearColor::White;
	}
	
	FText GetAssetDisplayName() const override
	{
		return INVTEXT("Epith Panels");
	}
	
	TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { FEpithAssetCategoryPaths::Epith };

		return Categories;
	}
};

UCLASS()
class UFactory_EpithPanelDefinition : public UFactory
{
	GENERATED_BODY()
	
	UFactory_EpithPanelDefinition()
	{
		SupportedClass = UEpithPanelDefinition::StaticClass();
		bCreateNew = true;
		bEditorImport = false;
		bEditAfterNew = true;
	}
	
	UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override
	{
		return NewObject<UEpithPanelDefinition>(InParent, InClass, InName, Flags | RF_Transactional);
	}
};