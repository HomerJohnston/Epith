#pragma once

#include "EpithWindowElement.h"
#include "Epith/Misc/EpithPropertySelector.h"

#include "EpithWindowElement_Property.generated.h"

USTRUCT(DisplayName = "Property")
struct FEpithWindowElement_Property : public FEpithWindowElement
{
	GENERATED_BODY()
	
	/** Property Path (this is just an FName; it is not bound to anything and will simply stop pointing to a property if the property is removed or renamed). */
	UPROPERTY(EditDefaultsOnly)
	FEpithPropertySelector PropertySelector;
	
	TSharedPtr<SWidget> Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const override;
	
	void RecursiveDrawRowChildren(TSharedPtr<FEpithPropertyDataContainer> InPropertyData, TSharedPtr<IDetailTreeNode> TreeNode, TArray<TSharedRef<SWidget>>& Widgets);
};