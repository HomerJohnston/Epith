#pragma once

#include "EpithWindowElement.h"
#include "Epith/Misc/EpithPropertySelector.h"

#include "EpithWindowElement_Property.generated.h"

/** How a property with children (a struct's members, an array's elements and so on) shows them. */
UENUM()
enum class EEpithPropertyChildDisplay : uint8
{
	/** Like the Details panel: an arrow on the property's row opens and closes its children. */
	Foldout,

	/** No arrow: the property's row is just its label (an array's keeps its element count and buttons), with its children always shown below. */
	AlwaysExpanded,

	/** No row for the property itself: only its children, as if they were properties of their own. */
	ChildrenOnly,
};

USTRUCT(DisplayName = "Property")
struct FEpithWindowElement_Property : public FEpithWindowElement
{
	GENERATED_BODY()
	
	/** Property Path (this is just an FName; it is not bound to anything and will simply stop pointing to a property if the property is removed or renamed). */
	UPROPERTY(EditDefaultsOnly)
	FEpithPropertySelector PropertySelector;

	/** For a property with children (a struct, an array and so on): a foldout, always shown, or only the children. */
	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	EEpithPropertyChildDisplay ChildDisplay = EEpithPropertyChildDisplay::Foldout;

	/** Applies Child Display all the way down: to children that have children of their own, and theirs. Otherwise they keep foldouts. */
	UPROPERTY(EditDefaultsOnly, Category = "Appearance", meta = (EditCondition = "ChildDisplay != EEpithPropertyChildDisplay::Foldout"))
	bool bRecursiveExpansion = false;
	
	TSharedPtr<SWidget> Draw(const FEpithDrawContext& Context) const override;
};