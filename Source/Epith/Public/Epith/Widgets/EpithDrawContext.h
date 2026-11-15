#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "Types/SlateStructs.h"

struct FEpithColumnSettings;
struct FEpithWindowElement;
struct FEpithPropertyDataContainer;
class SWidget;

/**
 * Edits a slot's owner (a group) can offer for that slot in the designer. Unset functions aren't offered.
 * Each one changes the panel definition directly; the designer runs them inside an undoable transaction.
 */
struct FEpithSlotActions
{
	TFunction<void()> Remove;
	TFunction<void()> MoveUp;
	TFunction<void()> MoveDown;
};

/**
 * Implemented by the panel designer (the panel definition's asset editor). Elements reach it through FEpithDrawContext
 * to add design-time widgets. In the level viewport there is no designer and elements draw exactly as before.
 */
class FEpithColumnLayout;

class IEpithDesigner
{
public:
	virtual ~IEpithDesigner() = default;

	/** A slot with nothing in it yet: a dashed box with a menu to choose what goes in it. */
	virtual TSharedRef<SWidget> MakeEmptySlot(TInstancedStruct<FEpithWindowElement>& Slot, const FEpithSlotActions& Actions) = 0;

	/** Wraps an element's widget with a hover outline and a menu (nest in a container, clear, remove, move). */
	virtual TSharedRef<SWidget> WrapElement(TInstancedStruct<FEpithWindowElement>& Slot, TSharedRef<SWidget> ElementWidget, const FEpithSlotActions& Actions) = 0;

	/** A small "+" button that runs AddSlot in a transaction, e.g. at the end of a group. */
	virtual TSharedRef<SWidget> MakeAddButton(const FText& Label, TFunction<void()> AddSlot) = 0;

	/** A dropdown for picking a property path on the panel's target class. OnPicked runs in a transaction. */
	virtual TSharedRef<SWidget> MakePropertyPicker(FName CurrentPath, TFunction<void(FName)> OnPicked) = 0;
	
	/** Drag handles over a property row's column lines, which resize the group's columns. Laid over the row's preview. */
	virtual TSharedRef<SWidget> MakeColumnHandles(const TSharedRef<FEpithColumnLayout>& Columns) = 0;
};

/**
 * Shared column widths for the property rows in one group, so their names and values line up.
 * The same idea as the Details panel's FDetailColumnSizeData: every row in the group reads its widths from one shared object.
 */
class FEpithColumnLayout
{
public:
	/** Row geometry shared by the rows and the designer's column handles: | Name | Value | Reset |, with 1px lines between. */
	static constexpr float LineWidth = 1.0f;
	static constexpr float ResetColumnWidth = 24.0f;
	
	/** @param InSourceSettings The group's saved settings these widths came from; the designer's column handles write to them. */
	EPITH_API explicit FEpithColumnLayout(float InFixedNameWidth = 0.0f, float InFixedValueWidth = 0.0f, const FEpithColumnSettings* InSourceSettings = nullptr);
	
	/** Live width changes while a designer column handle is dragged (saved to the settings when the drag ends). 0 = automatic. */
	void SetFixedNameWidth(float Width) { FixedNameWidth = Width; }
	void SetFixedValueWidth(float Width) { FixedValueWidth = Width; }
	
	const FEpithColumnSettings* GetSourceSettings() const { return SourceSettings; }

	/** Rows add their name area (indent, expander, edit-condition toggle, name) so the automatic width can fit the widest one. */
	EPITH_API void RegisterNameArea(const TSharedRef<SWidget>& NameArea);

	/** The fixed name width if one is set, otherwise the widest visible name area in the group (unset before anything has been measured). */
	EPITH_API FOptionalSize GetNameWidth() const;

	/** The fixed value width if one is set, otherwise unset (each value takes what it needs). */
	EPITH_API FOptionalSize GetValueWidth() const;

	/**
	 * Call right after building new rows (a panel, or a row's expanded children). Measures the new widgets now, so the automatic
	 * name width is right on the very first frame they're shown instead of settling a frame later (a visible flicker).
	 * Slate measures children before parents, so without this, rows further down the panel haven't been measured yet when the
	 * first row's column is sized.
	 */
	EPITH_API static void MeasureNow(const TSharedRef<SWidget>& NewContent);

private:
	float FixedNameWidth = 0.0f;
	float FixedValueWidth = 0.0f;
	
	const FEpithColumnSettings* SourceSettings = nullptr;

	TArray<TWeakPtr<SWidget>> NameAreas;

	/** The automatic width is worked out once per frame and shared by all rows. */
	mutable uint64 CachedFrame = MAX_uint64;
	mutable uint64 CachedGeneration = MAX_uint64;
	mutable float CachedNameWidth = 0.0f;

	/** Bumped by MeasureNow so cached widths from this frame are worked out again with the new measurements. */
	static uint64 Generation;
};

/** Everything an element needs to draw itself. */
struct FEpithDrawContext
{
	FEpithDrawContext() = default;

	explicit FEpithDrawContext(TSharedPtr<FEpithPropertyDataContainer> InPropertyData, TSharedPtr<IEpithDesigner> InDesigner = nullptr)
		: PropertyData(InPropertyData)
		, Designer(InDesigner)
	{
	}

	/** Property rows for the object being shown (a placed object in the viewport, the class defaults in the designer). */
	TSharedPtr<FEpithPropertyDataContainer> PropertyData;

	/** Only set in the designer. */
	TSharedPtr<IEpithDesigner> Designer;

	/** Column widths for property rows; set by the nearest enclosing group. Null outside any group (no alignment). */
	TSharedPtr<FEpithColumnLayout> Columns;

	/** A copy of this context for drawing a group's children, with a fresh column layout so the group's rows line up with each other. */
	EPITH_API FEpithDrawContext WithColumns(const FEpithColumnSettings& Settings) const;

	bool IsDesigning() const
	{
		return Designer.IsValid();
	}
};

namespace EpithDraw
{
	/**
	 * Draws whatever is in a slot. Groups use this for their children (and panes for the root).
	 * Outside the designer: the element's own Draw(), or null for an empty slot.
	 * In the designer: empty slots become pickers, and elements get designer chrome.
	 */
	EPITH_API TSharedPtr<SWidget> DrawSlot(const TInstancedStruct<FEpithWindowElement>& Slot, const FEpithDrawContext& Context, const FEpithSlotActions& Actions = FEpithSlotActions());

	/** Remove / move up / move down for entry Index of a group's slot array. */
	template<typename SlotType>
	FEpithSlotActions MakeArraySlotActions(TArray<SlotType>& Slots, int32 Index)
	{
		FEpithSlotActions Actions;
		TArray<SlotType>* SlotsPtr = &Slots;

		Actions.Remove = [SlotsPtr, Index]()
		{
			if (SlotsPtr->IsValidIndex(Index))
			{
				SlotsPtr->RemoveAt(Index);
			}
		};

		if (Index > 0)
		{
			Actions.MoveUp = [SlotsPtr, Index]()
			{
				if (SlotsPtr->IsValidIndex(Index) && SlotsPtr->IsValidIndex(Index - 1))
				{
					SlotsPtr->Swap(Index, Index - 1);
				}
			};
		}

		if (Index < Slots.Num() - 1)
		{
			Actions.MoveDown = [SlotsPtr, Index]()
			{
				if (SlotsPtr->IsValidIndex(Index) && SlotsPtr->IsValidIndex(Index + 1))
				{
					SlotsPtr->Swap(Index, Index + 1);
				}
			};
		}

		return Actions;
	}
}
