#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "Layout/Margin.h"
#include "Types/SlateStructs.h"

struct FEpithColumnSettings;
struct FEpithWindowElement;
struct FEpithPropertyDataContainer;
class SWidget;

/**
 * Edits a slot's owner (a group) can offer for that slot in the designer. Unset functions aren't offered.
 * Each one changes the panel definition directly; the designer runs them inside an undoable transaction.
 */
/** Which way the group an element sits in lays out its slots. */
enum class EEpithGroupAxis : uint8
{
	/** Not directly in a Vertical or Horizontal Group (the root, a foldout, a tab). */
	None,

	Vertical,

	Horizontal,
};

/** How the designer lets a group's slots be reordered by dragging. */
enum class EEpithSlotReorder : uint8
{
	/** Not draggable. */
	None,

	/** Slots are stacked top to bottom; dropping goes above or below a slot. */
	Vertical,

	/** Slots are in a row; dropping goes left or right of a slot. */
	Horizontal,
};

struct FEpithSlotActions
{
	/** Whether (and which way) the designer shows a grab handle to drag the slot to another position in its group. */
	EEpithSlotReorder Reorder = EEpithSlotReorder::None;

	/** The slot array this slot is in, to tell groups apart while dragging (slots only move within their own group). */
	const void* GroupId = nullptr;

	/** This slot's index in its group. */
	int32 Index = INDEX_NONE;

	/** The group's slot struct (e.g. a Vertical Group slot, with its Border and Padding) and this slot's data, for the designer's Element tab. */
	const UScriptStruct* SlotStruct = nullptr;
	void* SlotMemory = nullptr;

	/** Moves the group's slot at From so it ends up in front of the slot that's at InsertBefore now (Num = to the end). */
	TFunction<void(int32 From, int32 InsertBefore)> MoveSlot;

	/** The group the slot belongs to, for tooltips (e.g. "Vertical Group"). */
	FText ParentName;

	TFunction<void()> Remove;
	TFunction<void()> MoveUp;
	TFunction<void()> MoveDown;
};

/**
 * Implemented by the panel designer (the panel definition's asset editor). Elements reach it through FEpithDrawContext
 * to add design-time widgets. In the level viewport there is no designer and elements draw exactly as before.
 */
class FEpithColumnLayout;

/** Makes a new slot in a group (at its end, or at a position) and returns where its element goes. */
using FEpithMakeSlot = TFunction<TInstancedStruct<FEpithWindowElement>*()>;

/** Dragging an insert divider sideways to move the boundary between the two slots either side of it (Horizontal Group). */
struct FEpithDividerResize
{
	/** Whether the slots either side can be resized; unset = they can't, and the divider only inserts. */
	TFunction<bool()> CanResize;

	/** The drag began. */
	TFunction<void()> Begin;

	/** The cursor is Delta from where the drag began, in the group's units (already slowed by the designer's drag speed). */
	TFunction<void(float Delta)> Update;

	/** The drag ended: save the new sizes. Runs inside the designer's transaction. */
	TFunction<void()> Commit;

	/** The drag was cancelled: drop the live sizes. */
	TFunction<void()> Cancel;
};

class IEpithDesigner
{
public:
	virtual ~IEpithDesigner() = default;

	/** Around the drawing of each slot (EpithDraw::DrawSlot), so the designer knows which slots a widget it makes sits inside. */
	virtual void BeginSlot(const TInstancedStruct<FEpithWindowElement>& Slot) {}
	virtual void EndSlot() {}

	/** A slot with nothing in it yet: a dashed box with a menu to choose what goes in it. */
	virtual TSharedRef<SWidget> MakeEmptySlot(TInstancedStruct<FEpithWindowElement>& Slot, const FEpithSlotActions& Actions) = 0;

	/** Wraps an element's widget with a hover outline and a menu (nest in a container, clear, remove, move). */
	virtual TSharedRef<SWidget> WrapElement(TInstancedStruct<FEpithWindowElement>& Slot, TSharedRef<SWidget> ElementWidget, const FEpithSlotActions& Actions) = 0;

	/**
	 * A "+" icon button that runs AddSlot in a transaction, e.g. at the end of a group. Label is its tooltip.
	 * An element dragged onto it moves into the new slot AddSlot makes.
	 */
	virtual TSharedRef<SWidget> MakeAddButton(const FText& Label, FEpithMakeSlot AddSlot) = 0;

	/** A dropdown for picking a property path on the panel's target class. OnPicked runs in a transaction. */
	virtual TSharedRef<SWidget> MakePropertyPicker(FName CurrentPath, TFunction<void(FName)> OnPicked) = 0;

	/**
	 * A thin divider between two slots that inserts a new slot there when clicked: a 1px line inside a 3px button.
	 * bRunsHorizontally: a horizontal line (between slots stacked vertically); otherwise a vertical line (between slots in a row).
	 * An element dragged onto it moves into the new slot InsertSlot makes. With Resize set (a vertical line), dragging it
	 * sideways resizes the slots either side instead.
	 */
	virtual TSharedRef<SWidget> MakeInsertDivider(bool bRunsHorizontally, const FText& ToolTip, FEpithMakeSlot InsertSlot, const FEpithDividerResize& Resize = FEpithDividerResize()) = 0;

	/** A "Pick control..." dropdown listing every special control type (subclasses of FEpithSpecialControl). */
	virtual TSharedRef<SWidget> MakeControlPicker(TFunction<void(const UScriptStruct*)> OnPicked) = 0;

	/** A drag handle over a property row's name/value line, which sets the group's name column width. Laid over the row's preview. */
	virtual TSharedRef<SWidget> MakeColumnHandles(const TSharedRef<FEpithColumnLayout>& Columns) = 0;
};

/**
 * Shared column widths for the property rows in one group, so their names and values line up.
 * The same idea as the Details panel's FDetailColumnSizeData: every row in the group reads its widths from one shared object.
 */
class FEpithColumnLayout
{
public:
	/** Row geometry shared by the rows and the designer's column handle: | Name | Value | Reset |, with 1px lines between. */
	static constexpr float LineWidth = 1.0f;
	static constexpr float ResetColumnWidth = 24.0f;

	/** The value column never gets narrower than this when the name column is widened. */
	static constexpr float MinValueWidth = 16.0f;

	/** The name column's share of the row is kept within these (0 = automatic, outside them). */
	static constexpr float MinNameFraction = 0.05f;
	static constexpr float MaxNameFraction = 0.95f;

	/** @param InSourceSettings The group's saved settings these widths came from; the designer's column handle writes to them. */
	EPITH_API explicit FEpithColumnLayout(float InNameFraction = 0.0f, const FEpithColumnSettings* InSourceSettings = nullptr);

	/** Live changes while the designer's column handle is dragged (saved to the settings when the drag ends). 0 = automatic. */
	EPITH_API void SetNameFraction(float Fraction);

	/** The name column's share of the row (0 = as wide as the widest name). */
	float GetNameFraction() const { return NameFraction; }

	const FEpithColumnSettings* GetSourceSettings() const { return SourceSettings; }

	/** Rows add their name area (indent, expander, edit-condition toggle, name) so the automatic width can fit the widest one. */
	EPITH_API void RegisterNameArea(const TSharedRef<SWidget>& NameArea);

	/**
	 * How wide the name column is in a row RowWidth wide: its share of the row, or the widest visible name area in the group if
	 * automatic. Always leaves room for the value and reset columns. Unset if automatic and nothing has been measured yet.
	 */
	EPITH_API TOptional<float> GetNameWidth(float RowWidth) const;

	/** The widest visible name area in the group (unset before anything has been measured). */
	EPITH_API TOptional<float> GetAutomaticNameWidth() const;

	/**
	 * Call right after building new rows (a panel, or a row's expanded children). Measures the new widgets now, so the automatic
	 * name width is right on the very first frame they're shown instead of settling a frame later (a visible flicker).
	 * Slate measures children before parents, so without this, rows further down the panel haven't been measured yet when the
	 * first row's column is sized.
	 */
	EPITH_API static void MeasureNow(const TSharedRef<SWidget>& NewContent);

private:
	float NameFraction = 0.0f;

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
/**
 * UI state that should survive a redraw of the same panel: which tab each Tabbed Group shows, and which Foldouts are open.
 * Kept by whoever redraws (the panel designer, a viewport pane) and passed down in the draw context.
 *
 * Keyed by the element's own data (its address): each element's instanced struct owns its memory, which stays put when the
 * element is moved or reordered, or when slots are added around it. A brand-new element (or an undo) just starts fresh.
 */
struct FEpithViewState
{
	TMap<const void*, int32> TabIndices;
	TMap<const void*, bool> FoldoutExpanded;
};

/** Which grid lines a property row draws along its top and bottom. (The lines between its columns are always drawn.) */
struct FEpithRowEdges
{
	/** A line along the top. Vertical Groups bind it to "no visible property row directly above this one". */
	TAttribute<bool> Top;

	/** A line along the bottom. Rows in a Vertical Group, and their child rows. */
	bool bBottom = false;

	/** Both lines: for a row on its own (directly in a Foldout, a Tabbed Group's tab, or at the root of a panel). */
	static FEpithRowEdges Standalone()
	{
		FEpithRowEdges Edges;
		Edges.Top.Set(true);
		Edges.bBottom = true;
		return Edges;
	}
};

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

	/** Tabs and foldouts to restore after a redraw. Null = everything starts in its default state. */
	TSharedPtr<FEpithViewState> ViewState;

	/** The layout of the group this element is directly in (e.g. so a Divider knows which way to be a gap). */
	EEpithGroupAxis ParentAxis = EEpithGroupAxis::None;

	/**
	 * How far past each of its edges this element's filled Divider bars should reach, to join the bar of a filled Divider in a
	 * neighbouring group (making T and + shapes). 0 on a side = no bar to join there.
	 *
	 * Set by Vertical and Horizontal Groups, one level deep: a group next to a filled divider gets the distance to its bar, and
	 * passes it (plus its own border and slot padding) on to its children across its own direction. Only dividers use it.
	 * Cleared by WithColumns, so it never reaches further down. Always 0 in the designer, whose chrome sits between slots.
	 */
	FMargin DividerReach = FMargin(0.0f);

	/**
	 * Top and bottom grid lines for a property row drawn in this slot. A row on its own (at the root, directly in a Foldout or a
	 * tab) gets both; Vertical Groups set them per slot so a run of rows shares single lines; Horizontal Groups give their rows
	 * none. Reset to both by WithColumns.
	 */
	FEpithRowEdges RowEdges = FEpithRowEdges::Standalone();

	/**
	 * A copy of this context for drawing a group's children, with a fresh column layout so the group's rows line up with each
	 * other, and the group's own axis as the children's ParentAxis.
	 */
	EPITH_API FEpithDrawContext WithColumns(const FEpithColumnSettings& Settings, EEpithGroupAxis ChildAxis = EEpithGroupAxis::None) const;

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

	/** True if the slot holds a Divider that draws its bar (bFilled), i.e. one that neighbouring dividers can join. */
	EPITH_API bool IsFilledDivider(const TInstancedStruct<FEpithWindowElement>& Slot);

	/** Remove / move up / move down for entry Index of a group's slot array. */
	template<typename SlotType>
	FEpithSlotActions MakeArraySlotActions(TArray<SlotType>& Slots, int32 Index, const FText& ParentName = FText(), EEpithSlotReorder Reorder = EEpithSlotReorder::None)
	{
		FEpithSlotActions Actions;
		Actions.ParentName = ParentName;
		Actions.Reorder = Reorder;
		Actions.GroupId = &Slots;
		Actions.Index = Index;
		Actions.SlotStruct = SlotType::StaticStruct();
		Actions.SlotMemory = Slots.IsValidIndex(Index) ? &Slots[Index] : nullptr;
		TArray<SlotType>* SlotsPtr = &Slots;

		Actions.MoveSlot = [SlotsPtr](int32 From, int32 InsertBefore)
		{
			if (!SlotsPtr->IsValidIndex(From) || InsertBefore < 0 || InsertBefore > SlotsPtr->Num() || InsertBefore == From || InsertBefore == From + 1)
			{
				return;
			}

			SlotType Moved = MoveTemp((*SlotsPtr)[From]);
			SlotsPtr->RemoveAt(From);
			SlotsPtr->Insert(MoveTemp(Moved), InsertBefore > From ? InsertBefore - 1 : InsertBefore);
		};

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
