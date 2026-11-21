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
	friend struct FEpithTestAccess; // Automation tests (Private/Epith/Tests)

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Epith", meta = (AllowAbstract))
	TSubclassOf<UObject> TargetClass;

	/** Ignore this panel definition entirely: it's never shown, and doesn't stop other definitions from being used. */
	UPROPERTY(EditDefaultsOnly, Category = "Epith")
	bool bDisabled = false;

	/**
	 * Normally only the most specific panel is shown for an object: a panel for Pawn is used instead of one for Actor.
	 * A supplementary panel is always shown, as an extra section, for every object of its target class or a subclass.
	 * An object's sections are ordered from the most general target class to the most specific (Actor, then Pawn, then Character).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Epith")
	bool bSupplementary = false;

	/** Heading shown above this object's section of the panel. Leave empty to use the object's class name (or the component's name, for components). */
	UPROPERTY(EditDefaultsOnly, Category = "Epith|Appearance")
	FText Title;

	/**
	 * Width of this object's section of the panel, in Slate units. Everything inside fits this width: property columns and
	 * Horizontal Group slots are fractions of it. (If sections of one panel differ, the panel is as wide as the widest.)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Epith|Appearance", meta = (ClampMin = 100, ClampMax = 2000, UIMin = 200, UIMax = 800))
	float Width = 360;

	/** Clicking this section's header in the viewport collapses or expands it (it starts expanded). */
	UPROPERTY(EditDefaultsOnly, Category = "Epith|Appearance")
	bool bFoldable = false;

	UPROPERTY(EditDefaultsOnly, Category = "Epith", meta = (ShowOnlyInnerProperties, ExcludeBaseStruct))
	TInstancedStruct<FEpithWindowElement> Root;

public:
	TSubclassOf<UObject> GetTargetClass() const
	{
		return TargetClass;
	}

	bool IsSupplementary() const
	{
		return bSupplementary;
	}

	bool IsDisabled() const
	{
		return bDisabled;
	}

	const FText& GetTitle() const
	{
		return Title;
	}

	static constexpr float MinPanelWidth = 100.0f;
	static constexpr float MaxPanelWidth = 2000.0f;

	bool IsFoldable() const
	{
		return bFoldable;
	}

	float GetWidth() const
	{
		return FMath::Clamp(Width, MinPanelWidth, MaxPanelWidth);
	}

	/** For the panel designer (wrap in a transaction and call Modify() first). */
	void SetWidth(float InWidth)
	{
		Width = FMath::Clamp(InWidth, MinPanelWidth, MaxPanelWidth);
	}

	const TInstancedStruct<FEpithWindowElement>* GetRoot() const
	{
		if (Root.IsValid())
		{
			return &Root;
		}

		return nullptr;
	}

	/** The root slot, for the panel designer to edit in place (wrap changes in a transaction and call Modify() first). */
	TInstancedStruct<FEpithWindowElement>& GetRootForEditing()
	{
		return Root;
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

	/** Opens the Epith panel editor (Details + panel designer). */
	EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
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