#pragma once

#include "EpithWindowElement.h"

#include "EpithWindowElement_Foldout.generated.h"

/** A collapsible section with a title, holding one element. */
USTRUCT(DisplayName = "Foldout")
struct FEpithWindowElement_Foldout : public FEpithWindowElement
{
	GENERATED_BODY()

	virtual ~FEpithWindowElement_Foldout() {}

	UPROPERTY(EditDefaultsOnly)
	FString FoldoutTitle;

	/** Column widths for the property rows in this foldout. */
	UPROPERTY(EditDefaultsOnly)
	FEpithColumnSettings Columns;
	
	UPROPERTY(EditDefaultsOnly, meta = (ShowOnlyInnerProperties, ExcludeBaseStruct))
	TInstancedStruct<FEpithWindowElement> Element;

	TSharedPtr<SWidget> Draw(const FEpithDrawContext& Context) const override;
};
