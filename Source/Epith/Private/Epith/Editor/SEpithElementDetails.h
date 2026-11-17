#pragma once

#include "Misc/NotifyHook.h"
#include "StructUtils/InstancedStruct.h"
#include "Widgets/SCompoundWidget.h"

class IStructureDetailsView;
class STextBlock;
class UEpithPanelDefinition;
struct FEpithWindowElement;

/**
 * The panel designer side pane's ELEMENT section: a small Details view for whatever is selected in the preview.
 *
 * Shows two sections: the slot's own settings (e.g. a Vertical Group slot's Border and Padding, a tab's name), if the element
 * sits in a group, and the element's settings. Child elements are hidden here; select them in the designer instead.
 *
 * The views edit the definition's data in place. Every change is wrapped in an undoable transaction on the definition.
 */
class SEpithElementDetails : public SCompoundWidget, public FNotifyHook
{
public:
	SLATE_BEGIN_ARGS(SEpithElementDetails) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEpithPanelDefinition* InDefinition);

	/**
	 * Shows Element (and its slot, if SlotStruct/SlotMemory are set). Null = nothing selected.
	 * If it's already showing exactly this, only refreshes the values (unless the change came from this tab itself).
	 */
	void SetSelection(TInstancedStruct<FEpithWindowElement>* Element, const UScriptStruct* SlotStruct, void* SlotMemory);

	/**
	 * Stops showing the data until the next SetSelection. Call before the definition's layout changes (adding, moving, removing
	 * elements; undo), since that can move the data these views point at.
	 */
	void Suspend();

	/** True while this tab is reporting one of its own changes (so the designer doesn't suspend it for that). */
	bool IsApplyingChange() const { return bApplyingChange; }

	// FNotifyHook
	void NotifyPreChange(FProperty* PropertyAboutToChange) override;
	void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;

private:
	TSharedRef<IStructureDetailsView> MakeStructView();

	void ShowNothing();

	TWeakObjectPtr<UEpithPanelDefinition> Definition;

	TSharedPtr<IStructureDetailsView> SlotView;
	TSharedPtr<IStructureDetailsView> ElementView;

	TSharedPtr<STextBlock> SlotHeading;
	TSharedPtr<STextBlock> ElementHeading;

	// What's shown, to tell a new selection from a refresh of the same one
	const UScriptStruct* ShownSlotStruct = nullptr;
	void* ShownSlotMemory = nullptr;
	const UScriptStruct* ShownElementStruct = nullptr;
	void* ShownElementMemory = nullptr;
	bool bShowingSomething = false;
	bool bSuspended = false;

	bool bApplyingChange = false;

	/** The last change came from this tab; the redraw it causes doesn't need to refresh the views. */
	bool bOwnChangePending = false;

	/** A transaction this tab opened (when the details view didn't already have one open). */
	bool bOwnTransaction = false;
};
