#pragma once

#include "EditorUndoClient.h"
#include "Epith/Widgets/EpithDrawContext.h"
#include "Widgets/SCompoundWidget.h"

class SBox;
class SButton;
class SEpithPropertyPalette;

/** What the innermost hovered element or slot in the preview is, for the label under the panel. Written by its frame as it paints. */
struct FEpithDesignerHoverNote
{
	FText Text;

	/** The frame (GFrameCounter) it was last written in; anything older means nothing is hovered now. */
	uint64 FrameNumber = 0;
};
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
 * - A pane pulls out from the left (its tab is on the preview's left edge), with ELEMENTS and PROPERTIES: drag an element type
 *   or a property from it onto an empty slot.
 * - A side pane on the right holds foldout sections; ELEMENT shows the selected element's settings (SEpithElementDetails).
 */
class SEpithPanelDesigner : public SCompoundWidget, public IEpithDesigner, public FEditorUndoClient
{
public:
	SLATE_BEGIN_ARGS(SEpithPanelDesigner) {}

		/** Called after the designer changed the definition, e.g. so the Details tab can refresh. */
		SLATE_EVENT(FSimpleDelegate, OnDefinitionEdited)

		/** The selected element's settings, shown in the side pane's ELEMENT section. */
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

	/** Selects a slot (or nothing) and shows it in the ELEMENT section. */
	void Select(TInstancedStruct<FEpithWindowElement>* Slot);

	/** After a redraw: keeps the selection if its slot is still in the panel, otherwise clears it. */
	void UpdateSelection();

	/** Remembers a slot drawn in this redraw, with its group slot data, so it can be selected. */
	void NoteDrawnSlot(TInstancedStruct<FEpithWindowElement>& Slot, const FEpithSlotActions& Actions);

	/** Stops the ELEMENT section from showing data that's about to move (layout edits, undo). */
	void SuspendElementDetails();

	/** The Properties tab: a folder tab on the preview's left edge that pulls the Properties pane out, or pushes it back. */
	TSharedRef<SWidget> MakePaletteTab();

	/** Opens or closes the Properties pane, sliding it (bAnimate) or at once. Remembered between sessions. */
	void SetPaletteOpen(bool bOpen, bool bAnimate);

	EActiveTimerReturnType AnimatePalette(double InCurrentTime, float InDeltaTime);

	/** The REAL VIEW tab: a folder tab centred on the designer's bottom edge that pulls the real view up, or pushes it back down. */
	TSharedRef<SWidget> MakeRealViewTab();

	/** Shows or hides the real view in the lower half, sliding it (bAnimate) or at once. Remembered between sessions. */
	void SetRealViewOpen(bool bOpen, bool bAnimate);

	EActiveTimerReturnType AnimateRealView(double InCurrentTime, float InDeltaTime);

	/** Draws the panel again in the real view, as the level viewport does (no designer controls). Nothing while it's put away. */
	void RedrawRealView();

	/** The pane on the left (pulled out by its tab): ELEMENTS and PROPERTIES foldouts, whose rows are dragged onto empty slots. */
	TSharedRef<SWidget> MakeLeftPane();

	/** "Property: Health - Slot 3 of Vertical Group": what a slot holds, and where it is. Shown under the panel while hovered. */
	FText DescribeSlot(const TInstancedStruct<FEpithWindowElement>& Slot, const FEpithSlotActions& Actions) const;

	/** The side pane on the right: a scrolling column of foldout sections (ELEMENT, for now). */
	TSharedRef<SWidget> MakeSidePane();

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

	/** The Properties pane on the left: hidden to start with (unless it was open last time), slid out by its tab. */
	TSharedPtr<SEpithPropertyPalette> Palette;

	/** The lower half of the preview: the panel as the level viewport draws it. Hidden to start with (unless it was up last time). */
	TSharedPtr<SBox> RealViewArea;
	TSharedPtr<SButton> RealViewTabButton;
	bool bRealViewOpen = false;

	/** How far the real view is pulled up, 0 (hidden) to 1 (exactly half the height). */
	float RealViewOpenness = 0.0f;

	/** True while it's sliding (AnimateRealView is running). */
	bool bRealViewSliding = false;

	/** Written by the hovered frame each frame it's painted; read by the label under the panel. */
	TSharedRef<FEpithDesignerHoverNote> HoverNote = MakeShared<FEpithDesignerHoverNote>();
	TSharedPtr<SButton> PaletteTabButton;
	bool bPaletteOpen = false;

	/** How far the pane is slid out, 0 (hidden) to 1 (open): it moves toward bPaletteOpen when the tab is clicked. */
	float PaletteOpenness = 0.0f;

	/** True while the pane is sliding (AnimatePalette is running). */
	bool bPaletteSliding = false;

	/** The pane's width in pixels when it's open (the splitter beside it sets this; resizing the window doesn't). */
	float PaletteWidth = 260.0f;

	static constexpr float MinPaletteWidth = 120.0f;

	/** The side pane's width in pixels (the splitter beside it sets this; resizing the window doesn't). Remembered between sessions. */
	float SidePaneWidth = 320.0f;

	/** Whether the left pane's ELEMENTS and PROPERTIES sections are open. Remembered between sessions. */
	TSharedRef<bool> bElementsSectionExpanded = MakeShared<bool>(true);
	TSharedRef<bool> bPropertiesSectionExpanded = MakeShared<bool>(true);

	/** Whether the side pane's ELEMENT section is open. Remembered between sessions. */
	TSharedRef<bool> bElementSectionExpanded = MakeShared<bool>(true);

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
