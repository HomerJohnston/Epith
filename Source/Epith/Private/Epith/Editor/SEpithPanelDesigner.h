#pragma once

#include "EditorUndoClient.h"
#include "Epith/Widgets/EpithDrawContext.h"
#include "Widgets/SCompoundWidget.h"

class SBox;
class UEpithPanelDefinition;
struct FEpithPropertyDataContainer;

/**
 * The panel designer: a live preview of a panel definition that you build by clicking in it.
 *
 * - The top row picks the Target Class. Until one is picked, there's nothing to design.
 * - Below it, the panel is drawn the way the viewport draws it, but every slot gets designer chrome (see IEpithDesigner):
 *   empty slots are dashed boxes with an element picker, elements have a hover outline and a menu, and groups get "+ Add".
 * - Property rows preview the class defaults and can't be edited here.
 * - Every change is an undoable transaction on the panel definition; the preview redraws after each one.
 */
class SEpithPanelDesigner : public SCompoundWidget, public IEpithDesigner, public FEditorUndoClient
{
public:
	SLATE_BEGIN_ARGS(SEpithPanelDesigner) {}

		/** Called after the designer changed the definition, e.g. so the Details tab can refresh. */
		SLATE_EVENT(FSimpleDelegate, OnDefinitionEdited)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEpithPanelDefinition* InDefinition);

	~SEpithPanelDesigner();

	/** Redraws on the next tick. Safe to call from inside one of the preview's own click handlers. */
	void RequestRedraw();

	// IEpithDesigner
	TSharedRef<SWidget> MakeEmptySlot(TInstancedStruct<FEpithWindowElement>& Slot, const FEpithSlotActions& Actions) override;
	TSharedRef<SWidget> WrapElement(TInstancedStruct<FEpithWindowElement>& Slot, TSharedRef<SWidget> ElementWidget, const FEpithSlotActions& Actions) override;
	TSharedRef<SWidget> MakeAddButton(const FText& Label, TFunction<void()> AddSlot) override;
	TSharedRef<SWidget> MakePropertyPicker(FName CurrentPath, TFunction<void(FName)> OnPicked) override;
	TSharedRef<SWidget> MakeColumnHandles(const TSharedRef<FEpithColumnLayout>& Columns) override;

	// FEditorUndoClient
	void PostUndo(bool bSuccess) override;
	void PostRedo(bool bSuccess) override;

protected:
	/** Runs Mutation as one undoable change to the definition, then redraws. */
	void Edit(const FText& Description, TFunction<void()> Mutation);

	void Redraw();

	EActiveTimerReturnType HandleRedrawTimer(double InCurrentTime, float InDeltaTime);

	void OnObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);

	/** Menu for an element: nest in a container, clear, remove from group, move up/down. */
	TSharedRef<SWidget> MakeElementMenu(TInstancedStruct<FEpithWindowElement>* Slot, const FEpithSlotActions& Actions);

	/** Menu for an empty slot: pick an element type. */
	TSharedRef<SWidget> MakeElementTypeMenu(TInstancedStruct<FEpithWindowElement>* Slot);

	TWeakObjectPtr<UEpithPanelDefinition> Definition;

	FSimpleDelegate OnDefinitionEdited;

	TSharedPtr<SBox> PreviewArea;

	/** Class-default property rows for the preview. Remade on each redraw (a row's widgets can only be shown once). */
	TSharedPtr<FEpithPropertyDataContainer> PreviewData;

	bool bRedrawPending = false;
};
