#pragma once

#include "CoreMinimal.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "EpithPropertyMenu.h"
#include "Widgets/SCompoundWidget.h"

class SVerticalBox;

/** A property path dragged out of the designer's Properties pane, to drop on an empty slot of the panel. */
class FEpithPropertyDragDrop : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FEpithPropertyDragDrop, FDecoratedDragDropOp)

	FName PropertyPath;

	/** The property's name as listed, for the drag's tooltip. */
	FText Label;

	static TSharedRef<FEpithPropertyDragDrop> New(FName InPropertyPath, const FText& InLabel);
};

/** An element type dragged out of the designer's ELEMENTS list, to drop on an empty slot of the panel. */
class FEpithElementTypeDragDrop : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FEpithElementTypeDragDrop, FDecoratedDragDropOp)

	const UScriptStruct* ElementType = nullptr;

	/** The type's display name, for the drag's tooltip. */
	FText Label;

	static TSharedRef<FEpithElementTypeDragDrop> New(const UScriptStruct* InElementType);
};

/**
 * One row of the designer's left pane (an element type or a property): highlighted on hover, with a grab cursor, and dragged
 * out with a left-click drag. Optional tree lines on its left, for nested properties.
 */
class SEpithPaletteRow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEpithPaletteRow)
		: _Icon(nullptr)
		, _TextOpacity(1.0f)
		{}
		SLATE_ARGUMENT(FText, Label)

		/** Shown left of the label (e.g. an element type's icon). Null = none. */
		SLATE_ARGUMENT(const FSlateBrush*, Icon)
		SLATE_ATTRIBUTE(FSlateColor, TextColor)
		SLATE_ARGUMENT(float, TextOpacity)
		SLATE_ATTRIBUTE(FText, RowToolTip)
		SLATE_ARGUMENT(TArray<EpithPropertyMenu::ETreeLine>, TreeColumns)

		/** Makes the drag operation when the row is dragged. */
		SLATE_ARGUMENT(TFunction<TSharedRef<FDragDropOperation>()>, OnBeginDrag)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	TFunction<TSharedRef<FDragDropOperation>()> OnBeginDrag;
};

/**
 * The list in the designer's PROPERTIES section: every property of the target class, listed like the property menu (one section
 * per class, nested properties under the property they're in, with tree lines). Each row can be dragged onto an empty slot to
 * make it a Property element showing that property. It doesn't scroll on its own: the pane it's in does.
 */
class SEpithPropertyPalette : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEpithPropertyPalette) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/**
	 * Lists TargetClass's properties. Only rebuilt if the class changed, or with bForce (e.g. when the pane is opened, in case an
	 * instance has been placed in the level since, which makes the list exact).
	 */
	void Refresh(UClass* TargetClass, bool bForce = false);

	/** Paths shown by Property elements in the panel; they're listed in green. */
	void SetUsedPaths(const TSet<FName>& InUsedPaths);

private:
	void Rebuild();

	TWeakObjectPtr<UClass> ListedClass;
	bool bHasListed = false;

	/** Shared with the rows, so their colors follow edits without rebuilding the list. */
	TSharedRef<TSet<FName>> UsedPaths = MakeShared<TSet<FName>>();

	TSharedPtr<SVerticalBox> ListBox;
};
