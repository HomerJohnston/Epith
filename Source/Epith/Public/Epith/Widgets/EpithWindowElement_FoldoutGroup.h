#pragma once

#include "EpithWindowElement.h"

#include "EpithWindowElement_FoldoutGroup.generated.h"

USTRUCT()
struct FEpithWindowElement_FoldoutGroup_Slot
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, DisplayName = "Element", meta = (ShowOnlyInnerProperties, ExcludeBaseStruct))
	TInstancedStruct<FEpithWindowElement> Child;
};

USTRUCT(DisplayName = "Foldout Group")
struct FEpithWindowElement_FoldoutGroup : public FEpithWindowElement
{
	GENERATED_BODY()
	
	virtual ~FEpithWindowElement_FoldoutGroup() {}
	
	UPROPERTY(EditDefaultsOnly)
	FString FoldoutTitle;
	
	UPROPERTY(EditDefaultsOnly, meta = (ShowOnlyInnerProperties, ExcludeBaseStruct))
	TInstancedStruct<FEpithWindowElement> Element;
	
	TSharedPtr<SWidget> Draw(TSharedPtr<FEpithPropertyDataContainer> InPropertyData) const override;
};
