#pragma once

#include "EpithWindowElement.h"

#include "EpithWindowElement_TabbedGroup.generated.h"

USTRUCT()
struct FEpithWindowElement_TabbedGroup_SlotDisplayParams
{
	GENERATED_BODY()
};

USTRUCT()
struct FEpithWindowElement_TabbedGroup_Slot
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FString TabName;

	//UPROPERTY(EditDefaultsOnly, DisplayName = "Slot Appearance")
	//FEpithWindowElement_TabbedGroup_SlotDisplayParams DisplayParams;

	UPROPERTY(EditDefaultsOnly, meta = (ShowOnlyInnerProperties, ExcludeBaseStruct))
	TInstancedStruct<FEpithWindowElement> Element;
};

USTRUCT(DisplayName = "Tabbed Group")
struct FEpithWindowElement_TabbedGroup : public FEpithWindowElement
{
	GENERATED_BODY()

	virtual ~FEpithWindowElement_TabbedGroup() {}

	TSharedPtr<SWidget> Draw(const FEpithDrawContext& Context) const override;

	/** Border below the tab bar, around the contents (open at the top, where the tabs are): 0 = none, 1 = thin, 2 = thick. */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = 0, ClampMax = 2, UIMin = 0, UIMax = 2))
	uint8 Border = 0;

	/** The old on/off border setting; old assets load it and PostSerialize turns it into Border = thin. */
	UPROPERTY()
	bool bBorder_DEPRECATED = false;

	/** Column widths for the property rows in this group. */
	UPROPERTY(EditDefaultsOnly)
	FEpithColumnSettings Columns;

	UPROPERTY(EditDefaultsOnly, DisplayName = "Tabbed Group Slots",meta = (ExcludeBaseStruct))
	TArray<FEpithWindowElement_TabbedGroup_Slot> Slots;

	/** Moves the old bBorder setting into Border. */
	void PostSerialize(const FArchive& Ar);
};

template<>
struct TStructOpsTypeTraits<FEpithWindowElement_TabbedGroup> : public TStructOpsTypeTraitsBase2<FEpithWindowElement_TabbedGroup>
{
	enum
	{
		WithPostSerialize = true,
	};
};

