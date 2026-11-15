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

	/** Border below the foldout bar, around the contents (open at the top, where the bar is): 0 = none, 1 = thin, 2 = thick. */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = 0, ClampMax = 2, UIMin = 0, UIMax = 2))
	uint8 Border = 0;

	/** The old on/off border setting; old assets load it and PostSerialize turns it into Border = thin. */
	UPROPERTY()
	bool bBorder_DEPRECATED = false;

	/** Column widths for the property rows in this foldout. */
	UPROPERTY(EditDefaultsOnly)
	FEpithColumnSettings Columns;

	UPROPERTY(EditDefaultsOnly, meta = (ShowOnlyInnerProperties, ExcludeBaseStruct))
	TInstancedStruct<FEpithWindowElement> Element;

	TSharedPtr<SWidget> Draw(const FEpithDrawContext& Context) const override;

	/** Moves the old bBorder setting into Border. */
	void PostSerialize(const FArchive& Ar);
};

template<>
struct TStructOpsTypeTraits<FEpithWindowElement_Foldout> : public TStructOpsTypeTraitsBase2<FEpithWindowElement_Foldout>
{
	enum
	{
		WithPostSerialize = true,
	};
};

