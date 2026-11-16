#pragma once

#include "EditorUndoClient.h"
#include "Epith/Widgets/EpithDrawContext.h"
#include "Widgets/SCompoundWidget.h"

class SBox;
class SEpithElementDetails;
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

		/** Where the selected element's settings are shown (the panel editor's Element tab). */
		SLATE_ARGUMENT(TSharedPtr<SEpithElementDetails>, ElementDetails)

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
	TSharedRef<SWidget> MakeControlPicker(TFunction<void(const UScriptStruct*)> OnPicked) override;
	TSharedRef<SWidget> MakeInsertDivider(bool bRunsHorizontally, const FText& ToolTip, TFunction<void()> InsertSlot) override;

	// FEditorUndoClient
	void PostUndo(bool bSuccess) override;
	void PostRedo(bool bSuccess) override;

	// Clicking the empty area around the preview clears the selection
	FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

protected:
	/** Runs Mutation as one undoable change to the definition, then redraws. */
	void Edit(const FText& Description, TFunction<void()> Mutation);

	void Redraw();

	EActiveTimerReturnType HandleRedrawTimer(double InCurrentTime, float InDeltaTime);

	void OnObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);

	/** Menu for an element: nest in a container, clear, remove from group, move up/down. */
	TSharedRef<SWidget> MakeElementMenu(TInstancedStruct<FEpithWindowElement>* Slot, const FEpithSlotActions& Actions);

	/** Puts a grab handle to the left of Content and makes it a drop target, if the slot's group can be reordered by dragging. */
	TSharedRef<SWidget> WithDragHandle(TSharedRef<SWidget> Content, const FEpithSlotActions& Actions, const FText& DragLabel);

	/** Menu for an empty slot: pick an element type. Spacers are only offered in Vertical and Horizontal Group slots. */
	TSharedRef<SWidget> MakeElementTypeMenu(TInstancedStruct<FEpithWindowElement>* Slot, bool bAllowSpacer);

	/** Selects a slot (or nothing) and shows it in the Element tab. */
	void Select(TInstancedStruct<FEpithWindowElement>* Slot);

	/** After a redraw: keeps the selection if its slot is still in the panel, otherwise clears it. */
	void UpdateSelection();

	/** Remembers a slot drawn in this redraw, with its group slot data, so it can be selected. */
	void NoteDrawnSlot(TInstancedStruct<FEpithWindowElement>& Slot, const FEpithSlotActions& Actions);

	/** Stops the Element tab from showing data that's about to move (layout edits, undo). */
	void SuspendElementDetails();

	TWeakObjectPtr<UEpithPanelDefinition> Definition;

	FSimpleDelegate OnDefinitionEdited;

	TSharedPtr<SBox> PreviewArea;

	/** Class-default property rows for the preview. Remade on each redraw (a row's widgets can only be shown once). */
	TSharedPtr<FEpithPropertyDataContainer> PreviewData;

	bool bRedrawPending = false;

	/**
	 * The add button a group just made, waiting for that group's frame. A group makes its add button after drawing its children,
	 * so the next WrapElement call is always for that group. While the button is hovered, the group's outline is emphasized.
	 */
	TWeakPtr<SWidget> PendingAddButton;

	TSharedPtr<SEpithElementDetails> ElementDetails;

	/** Which tabs and foldouts are showing in the preview, restored after every redraw while the editor is open. */
	TSharedRef<FEpithViewState> ViewState = MakeShared<FEpithViewState>();

	/** The selected slot, in the definition's data. Only compared by address until it's confirmed to still be drawn. */
	TInstancedStruct<FEpithWindowElement>* SelectedSlot = nullptr;

	struct FDrawnSlot
	{
		const UScriptStruct* SlotStruct = nullptr;
		void* SlotMemory = nullptr;
	};

	/** Every slot drawn in the last redraw, by address. */
	TMap<const TInstancedStruct<FEpithWindowElement>*, FDrawnSlot> DrawnSlots;
};
