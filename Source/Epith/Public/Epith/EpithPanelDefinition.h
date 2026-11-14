#pragma once

#include "AssetDefinitionDefault.h"
#include "Module/Epith.h"
#include "Widgets/EpithWindowElement.h"

#include "EpithPanelDefinition.generated.h" 

/** An asset describing the layout of a viewport panel for objects of TargetClass. Not intended to be subclassed. */
UCLASS()
class EPITH_API UEpithPanelDefinition : public UObject
{
	GENERATED_BODY()
	
	friend class FPropertyTypeCustomization_EpithPropertySelector;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Epith", meta = (AllowAbstract))
	TSubclassOf<UObject> TargetClass;
	
	/** Heading shown above this object's section of the panel. Leave empty to use the object's class name (or the component's name, for components). */
	UPROPERTY(EditDefaultsOnly, Category = "Epith|Appearance")
	FText Title;
	
	/** Minimum width of this object's section of the panel, in Slate units. */
	UPROPERTY(EditDefaultsOnly, Category = "Epith|Appearance", meta = (ClampMin = 0, UIMin = 100, UIMax = 600))
	float MinWidth = 180;
	
	UPROPERTY(EditDefaultsOnly, Category = "Epith", meta = (ShowOnlyInnerProperties, ExcludeBaseStruct))
	TInstancedStruct<FEpithWindowElement> Root;
	
public:
	TSubclassOf<UObject> GetTargetClass() const
	{
		return TargetClass;
	}
	
	const FText& GetTitle() const
	{
		return Title;
	}
	
	float GetMinWidth() const
	{
		return MinWidth;
	}
	
	const TInstancedStruct<FEpithWindowElement>* GetRoot() const
	{
		if (Root.IsValid())
		{
			return &Root;
		}
		
		return nullptr;
	}

	bool HasRoot() const
	{
		if (!Root.IsValid())
		{
			return false;
		}
		
		return true;
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