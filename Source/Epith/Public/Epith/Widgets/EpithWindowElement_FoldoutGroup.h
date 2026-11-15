#pragma once

#include "EpithWindowElement.h"

#include "EpithWindowElement_FoldoutGroup.generated.h"

USTRUCT(DisplayName = "Foldout Group")
struct FEpithWindowElement_FoldoutGroup : public FEpithWindowElement
{
	GENERATED_BODY()

	virtual ~FEpithWindowElement_FoldoutGroup() {}

	UPROPERTY(EditDefaultsOnly)
	FString FoldoutTitle;

	/** Column widths for the property rows in this group. */
	UPROPERTY(EditDefaultsOnly)
	FEpithColumnSettings Columns;
	
	UPROPERTY(EditDefaultsOnly, meta = (ShowOnlyInnerProperties, ExcludeBaseStruct))
	TInstancedStruct<FEpithWindowElement> Element;

	TSharedPtr<SWidget> Draw(const FEpithDrawContext& Context) const override;
};
