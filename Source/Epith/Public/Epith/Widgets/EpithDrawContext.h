#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

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
