#include "SEpithPanelDesigner.h"

#include "EpithPropertyMenu.h"
#include "SEpithElementDetails.h"
#include "ISinglePropertyView.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Editor.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"
#include "Epith/Style/EpithStyle.h"
#include "Epith/Widgets/EpithDrawContext.h"
#include "Epith/Widgets/EpithWindowElement_Foldout.h"
#include "Epith/Widgets/EpithWindowElement_HorizontalGroup.h"
#include "Epith/Widgets/EpithWindowElement_Property.h"
#include "Epith/Widgets/EpithWindowElement_SpecialControl.h"
#include "Epith/Widgets/EpithWindowElement_Spacer.h"
#include "Epith/Widgets/EpithWindowElement_TabbedGroup.h"
#include "Epith/Widgets/EpithWindowElement_VerticalGroup.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "UObject/UObjectIterator.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "EpithDesigner"

// ================================================================================================
// Helpers
// ================================================================================================

namespace EpithDesigner
{
	/** What an empty slot can become, in menu order. */
	const TArray<const UScriptStruct*>& GetElementTypes()
	{
		static const TArray<const UScriptStruct*> Types =
		{
			FEpithWindowElement_Foldout::StaticStruct(),
			FEpithWindowElement_VerticalGroup::StaticStruct(),
			FEpithWindowElement_HorizontalGroup::StaticStruct(),
			FEpithWindowElement_TabbedGroup::StaticStruct(),
			FEpithWindowElement_Property::StaticStruct(),
			FEpithWindowElement_SpecialControl::StaticStruct(),
			FEpithWindowElement_Spacer::StaticStruct(),
		};

		return Types;
	}

	/** Every special control type: the subclasses of FEpithSpecialControl in all loaded modules, by name. */
	TArray<const UScriptStruct*> GetControlTypes()
	{
		TArray<const UScriptStruct*> Types;
		const UScriptStruct* Base = FEpithSpecialControl::StaticStruct();

		for (TObjectIterator<UScriptStruct> It; It; ++It)
		{
			const UScriptStruct* Type = *It;

			if (Type != Base && Type->IsChildOf(Base) && !Type->HasMetaData(TEXT("Hidden")))
			{
				Types.Add(Type);
			}
		}

		Types.Sort([](const UScriptStruct& A, const UScriptStruct& B)
		{
			return A.GetDisplayNameText().CompareTo(B.GetDisplayNameText()) < 0;
		});

		return Types;
	}

	/** Element types that can hold other elements (for "Nest in"). */
	const TArray<const UScriptStruct*>& GetContainerTypes()
	{
		static const TArray<const UScriptStruct*> Types =
		{
			FEpithWindowElement_Foldout::StaticStruct(),
			FEpithWindowElement_VerticalGroup::StaticStruct(),
			FEpithWindowElement_HorizontalGroup::StaticStruct(),
			FEpithWindowElement_TabbedGroup::StaticStruct(),
		};

		return Types;
	}

	/** The editor's dropdown style, tinted orange: used for the "Choose ..." pickers on things that still need filling in. */
	const FComboButtonStyle& GetChooseComboStyle()
	{
		static const FComboButtonStyle Style = []()
		{
			FComboButtonStyle ComboStyle = FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("ComboButton");
			FButtonStyle ButtonStyle = ComboStyle.ButtonStyle;

			ButtonStyle.Normal.TintColor = FSlateColor(FLinearColor::FromSRGBColor(FColor(140, 66, 14)));
			ButtonStyle.Hovered.TintColor = FSlateColor(FLinearColor::FromSRGBColor(FColor(176, 84, 20)));
			ButtonStyle.Pressed.TintColor = FSlateColor(FLinearColor::FromSRGBColor(FColor(116, 54, 10)));

			ComboStyle.SetButtonStyle(ButtonStyle);
			return ComboStyle;
		}();

		return Style;
	}

	/** Turns a slot into a new element of Type. Groups start with no slots; their "+ to ..." button adds them. */
	void InitializeElement(TInstancedStruct<FEpithWindowElement>& Slot, const UScriptStruct* Type)
	{
		Slot.InitializeAsScriptStruct(Type);

		if (FEpithWindowElement_Foldout* FoldoutElement = Slot.GetMutablePtr<FEpithWindowElement_Foldout>())
		{
			FoldoutElement->FoldoutTitle = TEXT("Foldout");
		}
	}

	/** Replaces the element in Slot with a new container of ContainerType that holds it. Siblings and the parent are untouched. */
	void NestIn(TInstancedStruct<FEpithWindowElement>& Slot, const UScriptStruct* ContainerType)
	{
		TInstancedStruct<FEpithWindowElement> Inner = Slot;

		Slot.InitializeAsScriptStruct(ContainerType);

		if (FEpithWindowElement_VerticalGroup* Vertical = Slot.GetMutablePtr<FEpithWindowElement_VerticalGroup>())
		{
			Vertical->Slots.AddDefaulted_GetRef().Child = MoveTemp(Inner);
		}
		else if (FEpithWindowElement_HorizontalGroup* Horizontal = Slot.GetMutablePtr<FEpithWindowElement_HorizontalGroup>())
		{
			Horizontal->Slots.AddDefaulted_GetRef().Child = MoveTemp(Inner);
		}
		else if (FEpithWindowElement_TabbedGroup* Tabbed = Slot.GetMutablePtr<FEpithWindowElement_TabbedGroup>())
		{
			FEpithWindowElement_TabbedGroup_Slot& TabSlot = Tabbed->Slots.AddDefaulted_GetRef();
			TabSlot.TabName = TEXT("Tab 1");
			TabSlot.Element = MoveTemp(Inner);
		}
		else if (FEpithWindowElement_Foldout* FoldoutElement = Slot.GetMutablePtr<FEpithWindowElement_Foldout>())
		{
			FoldoutElement->FoldoutTitle = TEXT("Foldout");
			FoldoutElement->Element = MoveTemp(Inner);
		}
	}
}

// ================================================================================================
// SEpithDesignerFrame: outline around a slot (dashed for empty slots, solid on hover for elements), plus a right-click menu
// ================================================================================================

class SEpithDesignerFrame : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEpithDesignerFrame)
		: _bDashed(false)
		, _bOnlyWhenHovered(false)
		, _bOutlineContent(false)
		, _FillColor(FLinearColor::Transparent)
		, _Padding(FMargin(2))
		{}

		SLATE_ARGUMENT(bool, bDashed)
		SLATE_ARGUMENT(bool, bOnlyWhenHovered)

		/** Outline the content rather than the whole frame, so the padding stays outside the outline. */
		SLATE_ARGUMENT(bool, bOutlineContent)

		/** While this widget (the group's add button) is hovered, the outline is drawn brighter. */
		SLATE_ARGUMENT(TWeakPtr<SWidget>, EmphasizeWhileHovered)

		/** Background drawn inside the outline, behind the content (transparent = none). */
		SLATE_ARGUMENT(FLinearColor, FillColor)

		/** Left-click: select this element. */
		SLATE_EVENT(FSimpleDelegate, OnSelect)

		/** Selected elements always show a thicker, brighter outline. */
		SLATE_ATTRIBUTE(bool, IsSelected)
		SLATE_ARGUMENT(FMargin, Padding)
		SLATE_EVENT(FOnGetContent, OnGetContextMenu)
		SLATE_DEFAULT_SLOT(FArguments, Content)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		bDashed = InArgs._bDashed;
		bOnlyWhenHovered = InArgs._bOnlyWhenHovered;
		bOutlineContent = InArgs._bOutlineContent;
		EmphasizeWhileHovered = InArgs._EmphasizeWhileHovered;
		FillColor = InArgs._FillColor;
		OnSelect = InArgs._OnSelect;
		IsSelected = InArgs._IsSelected;
		OnGetContextMenu = InArgs._OnGetContextMenu;

		ChildSlot
		.Padding(InArgs._Padding)
		[
			InArgs._Content.Widget
		];
	}

	int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		if (FillColor.A > 0.0f)
		{
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None, FillColor);
		}

		const int32 MaxLayer = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 1, InWidgetStyle, bParentEnabled);

		const bool bSelected = IsSelected.Get(false);

		if (bOnlyWhenHovered && !IsHovered() && !bSelected)
		{
			return MaxLayer;
		}

		const TSharedPtr<SWidget> EmphasisSource = EmphasizeWhileHovered.Pin();
		const bool bEmphasized = EmphasisSource.IsValid() && EmphasisSource->IsHovered();

		const FSlateBrush* Brush = FAppStyle::GetBrush("WhiteBrush");
		FLinearColor Color = bDashed
			? FSlateColor(EStyleColor::Foreground).GetColor(InWidgetStyle).CopyWithNewOpacity(0.4f)
			: FSlateColor(EStyleColor::Primary).GetColor(InWidgetStyle);

		if (bSelected)
		{
			Color = FStyleColors::Select.GetColor(InWidgetStyle);
		}

		if (bEmphasized)
		{
			// The same blue, lifted toward white
			Color = FMath::Lerp(Color, FLinearColor::White, 0.45f).CopyWithNewOpacity(1.0f);
		}

		// Either the whole frame, or just the content inside the padding
		FGeometry OutlineGeometry = AllottedGeometry;

		if (bOutlineContent)
		{
			FArrangedChildren ArrangedChildren(EVisibility::Visible);
			ArrangeChildren(AllottedGeometry, ArrangedChildren);

			if (ArrangedChildren.Num() > 0)
			{
				OutlineGeometry = ArrangedChildren[0].Geometry;
			}
		}

		const FVector2f Size(OutlineGeometry.GetLocalSize());
		const int32 OutlineLayer = MaxLayer + 1;

		auto DrawRect = [&](float X, float Y, float W, float H)
		{
			FSlateDrawElement::MakeBox(OutDrawElements, OutlineLayer, OutlineGeometry.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f(X, Y))), Brush, ESlateDrawEffect::None, Color);
		};

		const float Thickness = bSelected ? 2.0f : 1.0f;

		if (!bDashed || bSelected)
		{
			DrawRect(0, 0, Size.X, Thickness);
			DrawRect(0, Size.Y - Thickness, Size.X, Thickness);
			DrawRect(0, 0, Thickness, Size.Y);
			DrawRect(Size.X - Thickness, 0, Thickness, Size.Y);
		}
		else
		{
			constexpr float Dash = 6.0f;
			constexpr float Gap = 4.0f;

			for (float X = 0; X < Size.X; X += Dash + Gap)
			{
				const float W = FMath::Min(Dash, Size.X - X);
				DrawRect(X, 0, W, Thickness);
				DrawRect(X, Size.Y - Thickness, W, Thickness);
			}

			for (float Y = 0; Y < Size.Y; Y += Dash + Gap)
			{
				const float H = FMath::Min(Dash, Size.Y - Y);
				DrawRect(0, Y, Thickness, H);
				DrawRect(Size.X - Thickness, Y, Thickness, H);
			}
		}

		return OutlineLayer;
	}

	FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		// Claim right-clicks so the innermost frame under the cursor gets the context menu
		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && OnGetContextMenu.IsBound())
		{
			return FReply::Handled();
		}

		// Left-click selects the innermost element under the cursor
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && OnSelect.IsBound())
		{
			OnSelect.Execute();
			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && OnGetContextMenu.IsBound())
		{
			const FWidgetPath WidgetPath = MouseEvent.GetEventPath() ? *MouseEvent.GetEventPath() : FWidgetPath();

			FSlateApplication::Get().PushMenu(AsShared(), WidgetPath, OnGetContextMenu.Execute(), MouseEvent.GetScreenSpacePosition(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));

			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

private:
	bool bDashed = false;
	bool bOnlyWhenHovered = false;
	bool bOutlineContent = false;
	TWeakPtr<SWidget> EmphasizeWhileHovered;
	FLinearColor FillColor = FLinearColor::Transparent;
	FSimpleDelegate OnSelect;
	TAttribute<bool> IsSelected;
	FOnGetContent OnGetContextMenu;
};

// ================================================================================================
// SEpithPanelDesigner
// ================================================================================================

// ------------------------------------------------------------------------------------------------

/** Dragging a slot to another position in its group. */
class FEpithSlotDragDrop : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FEpithSlotDragDrop, FDecoratedDragDropOp)

	/** The slot array the slot is in; it can only be dropped among slots of the same group. */
	const void* GroupId = nullptr;

	int32 Index = INDEX_NONE;

	static TSharedRef<FEpithSlotDragDrop> New(const void* InGroupId, int32 InIndex, const FText& Label)
	{
		TSharedRef<FEpithSlotDragDrop> Operation = MakeShared<FEpithSlotDragDrop>();
		Operation->GroupId = InGroupId;
		Operation->Index = InIndex;
		Operation->CurrentHoverText = Label;
		Operation->CurrentIconBrush = FAppStyle::GetBrush("VerticalBoxDragIndicatorShort");
		Operation->SetupDefaults();
		Operation->Construct();
		return Operation;
	}
};

/** The grip to the left of a slot: drag it to move the slot within its group. */
class SEpithSlotDragHandle : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEpithSlotDragHandle) {}
		SLATE_ARGUMENT(const void*, GroupId)
		SLATE_ARGUMENT(int32, Index)
		SLATE_ARGUMENT(FText, Label)
		/** The group the slot is in (e.g. "Vertical Group"), for the tooltip */
		SLATE_ARGUMENT(FText, ParentName)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		GroupId = InArgs._GroupId;
		Index = InArgs._Index;
		Label = InArgs._Label;

		SetCursor(EMouseCursor::GrabHand);
		SetToolTipText(InArgs._ParentName.IsEmpty()
			? LOCTEXT("DragHandle_Tooltip", "Drag Slot")
			: FText::Format(LOCTEXT("DragHandle_TooltipIn", "Drag {0} Slot"), InArgs._ParentName));

		ChildSlot
		[
			SNew(SBox)
			.WidthOverride(10.0f)
			.HeightOverride(20.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("VerticalBoxDragIndicatorShort"))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
		}

		return FReply::Unhandled();
	}

	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return FReply::Handled().BeginDragDrop(FEpithSlotDragDrop::New(GroupId, Index, Label));
	}

private:
	const void* GroupId = nullptr;
	int32 Index = INDEX_NONE;
	FText Label;
};

/**
 * Wraps a slot in the designer so another slot of the same group can be dropped before or after it, with a line showing where.
 * Drags from other groups pass through, so they can land on an enclosing slot of their own group instead.
 */
class SEpithSlotDropTarget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEpithSlotDropTarget) {}
		SLATE_ARGUMENT(const void*, GroupId)
		SLATE_ARGUMENT(int32, Index)
		SLATE_ARGUMENT(bool, bHorizontal)
		SLATE_ARGUMENT(TFunction<void(int32, int32)>, OnMove)
		SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		GroupId = InArgs._GroupId;
		Index = InArgs._Index;
		bHorizontal = InArgs._bHorizontal;
		OnMove = InArgs._OnMove;

		ChildSlot
		[
			InArgs._Content.Widget
		];
	}

	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		TSharedPtr<FEpithSlotDragDrop> Operation = DragDropEvent.GetOperationAs<FEpithSlotDragDrop>();

		if (!Operation.IsValid() || Operation->GroupId != GroupId)
		{
			bShowDropLine = false;
			return FReply::Unhandled();
		}

		// Before or after this slot, by which half of it the cursor is over
		const FVector2D Local = MyGeometry.AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
		const FVector2D Size = MyGeometry.GetLocalSize();
		bDropAfter = bHorizontal ? Local.X > Size.X * 0.5f : Local.Y > Size.Y * 0.5f;
		bShowDropLine = true;

		return FReply::Handled();
	}

	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override
	{
		bShowDropLine = false;
	}

	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		bShowDropLine = false;

		TSharedPtr<FEpithSlotDragDrop> Operation = DragDropEvent.GetOperationAs<FEpithSlotDragDrop>();

		if (!Operation.IsValid() || Operation->GroupId != GroupId)
		{
			return FReply::Unhandled();
		}

		const int32 InsertBefore = bDropAfter ? Index + 1 : Index;

		// Dropping a slot right where it already is changes nothing (and makes no undo step)
		if (OnMove && InsertBefore != Operation->Index && InsertBefore != Operation->Index + 1)
		{
			OnMove(Operation->Index, InsertBefore);
		}

		return FReply::Handled();
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		int32 MaxLayer = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

		// The drag ended somewhere else (or was cancelled) without a drag-leave reaching us
		if (bShowDropLine && !FSlateApplication::Get().IsDragDropping())
		{
			bShowDropLine = false;
		}

		if (bShowDropLine)
		{
			constexpr float Thickness = 2.0f;
			const FVector2f Size = FVector2f(AllottedGeometry.GetLocalSize());

			const FVector2f LineSize = bHorizontal ? FVector2f(Thickness, Size.Y) : FVector2f(Size.X, Thickness);
			const FVector2f LinePosition = bHorizontal
				? FVector2f(bDropAfter ? Size.X - Thickness : 0.0f, 0.0f)
				: FVector2f(0.0f, bDropAfter ? Size.Y - Thickness : 0.0f);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				MaxLayer + 1,
				AllottedGeometry.ToPaintGeometry(LineSize, FSlateLayoutTransform(LinePosition)),
				FAppStyle::GetBrush("WhiteBrush"),
				ESlateDrawEffect::None,
				FStyleColors::Select.GetColor(InWidgetStyle));

			MaxLayer += 1;
		}

		return MaxLayer;
	}

private:
	const void* GroupId = nullptr;
	int32 Index = INDEX_NONE;
	bool bHorizontal = false;
	TFunction<void(int32, int32)> OnMove;

	mutable bool bShowDropLine = false;
	bool bDropAfter = false;
};

void SEpithPanelDesigner::Construct(const FArguments& InArgs, UEpithPanelDefinition* InDefinition)
{
	Definition = InDefinition;
	OnDefinitionEdited = InArgs._OnDefinitionEdited;
	ElementDetails = InArgs._ElementDetails;

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FSinglePropertyParams TargetClassParams;
	TargetClassParams.NameOverride = LOCTEXT("TargetClass", "Target Class");

	// TargetClass is protected on the definition, so it's found by name. Edits through this view are undoable and broadcast a property change.
	TSharedPtr<ISinglePropertyView> TargetClassView = InDefinition ? PropertyEditor.CreateSingleProperty(InDefinition, TEXT("TargetClass"), TargetClassParams) : nullptr;

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8)
		[
			TargetClassView.IsValid() ? TargetClassView.ToSharedRef() : SNullWidget::NullWidget
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSeparator)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			.Padding(16)
			[
				SAssignNew(PreviewArea, SBox)
			]
		]
	];

	FCoreUObjectDelegates::OnObjectPropertyChanged.AddSP(this, &SEpithPanelDesigner::OnObjectPropertyChanged);

	if (GEditor)
	{
		GEditor->RegisterForUndo(this);
	}

	Redraw();
}

SEpithPanelDesigner::~SEpithPanelDesigner()
{
	FCoreUObjectDelegates::OnObjectPropertyChanged.RemoveAll(this);

	// FEditorUndoClient unregisters itself
}

void SEpithPanelDesigner::RequestRedraw()
{
	if (bRedrawPending)
	{
		return;
	}

	bRedrawPending = true;

	RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateSP(this, &SEpithPanelDesigner::HandleRedrawTimer));
}

EActiveTimerReturnType SEpithPanelDesigner::HandleRedrawTimer(double InCurrentTime, float InDeltaTime)
{
	bRedrawPending = false;

	Redraw();

	return EActiveTimerReturnType::Stop;
}

void SEpithPanelDesigner::Redraw()
{
	// Let go of the old preview first: its property rows belong to the old PreviewData
	PreviewArea->SetContent(SNullWidget::NullWidget);
	PendingAddButton.Reset();
	PreviewData.Reset();
	DrawnSlots.Reset();

	UEpithPanelDefinition* PanelDefinition = Definition.Get();

	if (!PanelDefinition)
	{
		return;
	}

	UClass* TargetClass = PanelDefinition->GetTargetClass();

	if (!TargetClass)
	{
		PreviewArea->SetContent(
			SNew(STextBlock)
			.Text(LOCTEXT("PickTargetClass", "Pick a Target Class above to start building the panel."))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground()));

		UpdateSelection();
		return;
	}

	// The preview shows the class defaults: values don't matter here, only the layout
	PreviewData = MakeShared<FEpithPropertyDataContainer>(TargetClass->GetDefaultObject());

	FEpithDrawContext Context(PreviewData, SharedThis(this));
	Context.ViewState = ViewState;

	TSharedPtr<SWidget> Content = EpithDraw::DrawSlot(PanelDefinition->GetRootForEditing(), Context);

	const FText Title = PanelDefinition->GetTitle().IsEmpty() ? TargetClass->GetDisplayNameText() : PanelDefinition->GetTitle();

	// Styled like the object pane in the viewport, so what you build is what you get
	PreviewArea->SetContent(
		SNew(SBox)
		.HAlign(HAlign_Left)
		[
			// Same background as a section of the real panel
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
			.Padding(8)
			[
				SNew(SBox)
				.MinDesiredWidth(PanelDefinition->GetMinWidth())
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(12, 4, 12, 8)
					[
						SNew(STextBlock)
						.Text(Title)
						.Font(EpithFonts.Font_ViewportPopup_Title)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						Content.IsValid() ? Content.ToSharedRef() : SNullWidget::NullWidget
					]
				]
			]
		]);

	// Line up property columns on the first frame (no one-frame flicker)
	FEpithColumnLayout::MeasureNow(PreviewArea.ToSharedRef());

	UpdateSelection();
}

void SEpithPanelDesigner::Edit(const FText& Description, TFunction<void()> Mutation)
{
	UEpithPanelDefinition* PanelDefinition = Definition.Get();

	if (!PanelDefinition || !Mutation)
	{
		return;
	}

	// The mutation may move slot data around; the Element tab must not look at it until the redraw
	SuspendElementDetails();

	{
		const FScopedTransaction Transaction(Description);
		PanelDefinition->Modify();
		Mutation();
		PanelDefinition->PostEditChange();
	}

	OnDefinitionEdited.ExecuteIfBound();

	RequestRedraw();
}

void SEpithPanelDesigner::OnObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent)
{
	// Covers the Target Class picker, the Details tab, the Element tab and our own edits
	if (Object && Object == Definition.Get())
	{
		// A change from elsewhere may have moved slot data; changes made in the Element tab itself don't move anything
		if (!ElementDetails.IsValid() || !ElementDetails->IsApplyingChange())
		{
			SuspendElementDetails();
		}

		RequestRedraw();
	}
}

void SEpithPanelDesigner::PostUndo(bool bSuccess)
{
	SuspendElementDetails();
	OnDefinitionEdited.ExecuteIfBound();
	RequestRedraw();
}

void SEpithPanelDesigner::PostRedo(bool bSuccess)
{
	SuspendElementDetails();
	OnDefinitionEdited.ExecuteIfBound();
	RequestRedraw();
}

// ------------------------------------------------------------------------------------------------
// IEpithDesigner
// ------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SEpithPanelDesigner::MakeEmptySlot(TInstancedStruct<FEpithWindowElement>& Slot, const FEpithSlotActions& Actions)
{
	TInstancedStruct<FEpithWindowElement>* SlotPtr = &Slot;

	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		[
			SNew(SComboButton)
			.ComboButtonStyle(&EpithDesigner::GetChooseComboStyle())
			.ContentPadding(FMargin(6, 2))
			.OnGetMenuContent_Lambda([this, SlotPtr, bAllowSpacer = Actions.Reorder != EEpithSlotReorder::None]()
			{
				// Only Vertical and Horizontal Group slots are reorderable, and only those can hold a Spacer
				return MakeElementTypeMenu(SlotPtr, bAllowSpacer);
			})
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("EmptySlot_Choose", "Pick container..."))
			]
		];

	if (Actions.Remove)
	{
		TFunction<void()> Remove = Actions.Remove;

		Row->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(4, 0, 0, 0)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ContentPadding(2)
			.ToolTipText(Actions.ParentName.IsEmpty()
				? LOCTEXT("EmptySlot_Remove", "Remove Slot")
				: FText::Format(LOCTEXT("EmptySlot_RemoveFrom", "Remove Slot from {0}"), Actions.ParentName))
			.OnClicked_Lambda([this, Remove]()
			{
				Edit(LOCTEXT("EmptySlot_RemoveTransaction", "Remove Slot"), Remove);
				return FReply::Handled();
			})
			[
				SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.X"))
				.ColorAndOpacity(FSlateColor::UseForeground())
			]
		];
	}

	// Unassigned slots are tinted a faint yellow, so they stand out as still to be filled in
	NoteDrawnSlot(Slot, Actions);

	TSharedRef<SWidget> Frame = SNew(SEpithDesignerFrame)
		.bDashed(true)
		.FillColor(FLinearColor(1.0f, 0.85f, 0.2f, 0.08f))
		.OnSelect_Lambda([this, SlotPtr]()
		{
			Select(SlotPtr);
		})
		.IsSelected_Lambda([this, SlotPtr]()
		{
			return SelectedSlot == SlotPtr;
		})
		.Padding(FMargin(8, 6))
		[
			Row
		];

	return WithDragHandle(Frame, Actions, LOCTEXT("DragLabel_Empty", "Empty Slot"));
}

TSharedRef<SWidget> SEpithPanelDesigner::WrapElement(TInstancedStruct<FEpithWindowElement>& Slot, TSharedRef<SWidget> ElementWidget, const FEpithSlotActions& Actions)
{
	TInstancedStruct<FEpithWindowElement>* SlotPtr = &Slot;

	// If this element is a group, the add button it just made belongs to it
	TWeakPtr<SWidget> AddButton = PendingAddButton;
	PendingAddButton.Reset();

	NoteDrawnSlot(Slot, Actions);

	// Left-click selects (settings in the Element tab), right-click opens the menu. The outline shows while hovered or selected.
	TSharedRef<SWidget> Frame = SNew(SEpithDesignerFrame)
		.bOnlyWhenHovered(true)
		.bOutlineContent(true)
		.EmphasizeWhileHovered(AddButton)
		.OnSelect_Lambda([this, SlotPtr]()
		{
			Select(SlotPtr);
		})
		.IsSelected_Lambda([this, SlotPtr]()
		{
			return SelectedSlot == SlotPtr;
		})
		// Each nesting level is inset a little, so nested outlines don't sit on top of each other
		.Padding(FMargin(2))
		.OnGetContextMenu_Lambda([this, SlotPtr, Actions]()
		{
			return MakeElementMenu(SlotPtr, Actions);
		})
		[
			ElementWidget
		];

	// What the drag shows: the element type, and for a property its path
	FText DragLabel = Slot.GetScriptStruct() ? Slot.GetScriptStruct()->GetDisplayNameText() : LOCTEXT("DragLabel_Empty", "Empty Slot");

	if (const FEpithWindowElement_Property* PropertyElement = Slot.GetPtr<FEpithWindowElement_Property>())
	{
		const FName Path = PropertyElement->PropertySelector.GetPropertyPath();

		if (!Path.IsNone())
		{
			DragLabel = FText::Format(LOCTEXT("DragLabel_Property", "{0}: {1}"), DragLabel, FText::FromName(Path));
		}
	}

	return WithDragHandle(Frame, Actions, DragLabel);
}

void SEpithPanelDesigner::NoteDrawnSlot(TInstancedStruct<FEpithWindowElement>& Slot, const FEpithSlotActions& Actions)
{
	FDrawnSlot& Drawn = DrawnSlots.FindOrAdd(&Slot);
	Drawn.SlotStruct = Actions.SlotStruct;
	Drawn.SlotMemory = Actions.SlotMemory;
}

void SEpithPanelDesigner::Select(TInstancedStruct<FEpithWindowElement>* Slot)
{
	const FDrawnSlot* Drawn = Slot ? DrawnSlots.Find(Slot) : nullptr;
	SelectedSlot = Drawn ? Slot : nullptr;

	if (ElementDetails.IsValid())
	{
		ElementDetails->SetSelection(SelectedSlot, Drawn ? Drawn->SlotStruct : nullptr, Drawn ? Drawn->SlotMemory : nullptr);
	}
}

void SEpithPanelDesigner::UpdateSelection()
{
	// If the selected slot wasn't drawn this time it's gone (removed, undone...); if it was, its data may have moved, so pass it on again
	Select(SelectedSlot && DrawnSlots.Contains(SelectedSlot) ? SelectedSlot : nullptr);
}

void SEpithPanelDesigner::SuspendElementDetails()
{
	if (ElementDetails.IsValid())
	{
		ElementDetails->Suspend();
	}
}

FReply SEpithPanelDesigner::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Only reached when no element under the cursor took the click
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && SelectedSlot)
	{
		Select(nullptr);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

TSharedRef<SWidget> SEpithPanelDesigner::WithDragHandle(TSharedRef<SWidget> Content, const FEpithSlotActions& Actions, const FText& DragLabel)
{
	if (Actions.Reorder == EEpithSlotReorder::None || !Actions.MoveSlot)
	{
		return Content;
	}

	TFunction<void(int32, int32)> MoveSlot = Actions.MoveSlot;

	return SNew(SEpithSlotDropTarget)
		.GroupId(Actions.GroupId)
		.Index(Actions.Index)
		.bHorizontal(Actions.Reorder == EEpithSlotReorder::Horizontal)
		.OnMove([this, MoveSlot](int32 From, int32 InsertBefore)
		{
			Edit(LOCTEXT("MoveSlotTransaction", "Move Slot"), [MoveSlot, From, InsertBefore]()
			{
				MoveSlot(From, InsertBefore);
			});
		})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 2, 0)
			[
				SNew(SEpithSlotDragHandle)
				.GroupId(Actions.GroupId)
				.Index(Actions.Index)
				.Label(DragLabel)
				.ParentName(Actions.ParentName)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				Content
			]
		];
}

TSharedRef<SWidget> SEpithPanelDesigner::MakeAddButton(const FText& Label, TFunction<void()> AddSlot)
{
	// A 24x24 button with a centered 16x16 "+" icon; the label is the tooltip
	TSharedRef<SWidget> Button = SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ContentPadding(0)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.ToolTipText(Label)
		.OnClicked_Lambda([this, AddSlot]()
		{
			Edit(LOCTEXT("AddSlotTransaction", "Add Slot"), AddSlot);
			return FReply::Handled();
		})
		[
			SNew(SImage)
			.Image(FAppStyle::Get().GetBrush("Icons.Plus"))
			.DesiredSizeOverride(FVector2D(16.0f, 16.0f))
			.ColorAndOpacity(FSlateColor::UseForeground())
		];

	// Picked up by the group's frame in WrapElement
	PendingAddButton = Button;

	return SNew(SBox)
		.WidthOverride(24.0f)
		.HeightOverride(24.0f)
		[
			Button
		];
}

TSharedRef<SWidget> SEpithPanelDesigner::MakePropertyPicker(FName CurrentPath, TFunction<void(FName)> OnPicked)
{
	return SNew(SComboButton)
		.ComboButtonStyle(&EpithDesigner::GetChooseComboStyle())
		.ContentPadding(FMargin(4, 1))
		.OnGetMenuContent_Lambda([this, OnPicked]()
		{
			UEpithPanelDefinition* PanelDefinition = Definition.Get();
			UClass* TargetClass = PanelDefinition ? PanelDefinition->GetTargetClass().Get() : nullptr;

			return EpithPropertyMenu::Make(TargetClass, [this, OnPicked](FName Path)
			{
				Edit(LOCTEXT("SetProperty", "Set Property"), [OnPicked, Path]()
				{
					OnPicked(Path);
				});
			},
			EpithPropertyMenu::GatherUsedPaths(PanelDefinition));
		})
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text(CurrentPath.IsNone() ? LOCTEXT("PropertyPicker_None", "Choose property...") : FText::FromName(CurrentPath))
		];
}

// ------------------------------------------------------------------------------------------------
// Menus
// ------------------------------------------------------------------------------------------------

TSharedRef<SWidget> SEpithPanelDesigner::MakeElementTypeMenu(TInstancedStruct<FEpithWindowElement>* Slot, bool bAllowSpacer)
{
	FMenuBuilder Menu(true, nullptr);

	for (const UScriptStruct* Type : EpithDesigner::GetElementTypes())
	{
		if (Type == FEpithWindowElement_Spacer::StaticStruct() && !bAllowSpacer)
		{
			continue;
		}

		Menu.AddMenuEntry(
			Type->GetDisplayNameText(),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, Slot, Type]()
			{
				Edit(FText::Format(LOCTEXT("AddElement", "Add {0}"), Type->GetDisplayNameText()), [Slot, Type]()
				{
					EpithDesigner::InitializeElement(*Slot, Type);
				});
			})));
	}

	return Menu.MakeWidget();
}

TSharedRef<SWidget> SEpithPanelDesigner::MakeElementMenu(TInstancedStruct<FEpithWindowElement>* Slot, const FEpithSlotActions& Actions)
{
	FMenuBuilder Menu(true, nullptr);

	const UScriptStruct* Type = Slot->GetScriptStruct();

	// --- The element itself: actions that only make sense for this kind of element

	FEpithWindowElement_Property* PropertyElement = Slot->GetMutablePtr<FEpithWindowElement_Property>();
	const bool bCanUnsetProperty = PropertyElement && !PropertyElement->PropertySelector.GetPropertyPath().IsNone();

	FEpithWindowElement_SpecialControl* ControlElement = Slot->GetMutablePtr<FEpithWindowElement_SpecialControl>();
	const bool bCanUnsetControl = ControlElement && ControlElement->Control.IsValid();

	if (Type && (bCanUnsetProperty || bCanUnsetControl))
	{
		Menu.BeginSection(NAME_None, FText::Format(LOCTEXT("ElementSection", "{0} Element"), Type->GetDisplayNameText()));

		// Unset to show the picker again and choose another
		if (bCanUnsetProperty)
		{
			Menu.AddMenuEntry(
				LOCTEXT("UnsetProperty", "Unset Property"),
				LOCTEXT("UnsetProperty_Tooltip", "Clear the chosen property, so you can choose a different one."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([this, PropertyElement]()
				{
					Edit(LOCTEXT("UnsetPropertyTransaction", "Unset Property"), [PropertyElement]()
					{
						PropertyElement->PropertySelector.SetPropertyPath(NAME_None);
					});
				})));
		}

		if (bCanUnsetControl)
		{
			Menu.AddMenuEntry(
				LOCTEXT("UnsetControl", "Unset Control"),
				LOCTEXT("UnsetControl_Tooltip", "Clear the chosen control, so you can choose a different one."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([this, ControlElement]()
				{
					Edit(LOCTEXT("UnsetControlTransaction", "Unset Control"), [ControlElement]()
					{
						ControlElement->Control.Reset();
					});
				})));
		}

		Menu.EndSection();
	}

	// --- The slot the element sits in: what goes in it, and where it is in its group

	Menu.BeginSection(NAME_None, Actions.ParentName.IsEmpty()
		? LOCTEXT("SlotSection", "Slot")
		: FText::Format(LOCTEXT("SlotSectionIn", "Slot in {0}"), Actions.ParentName));

	Menu.AddSubMenu(
		LOCTEXT("NestIn", "Nest In"),
		LOCTEXT("NestIn_Tooltip", "Put this element inside a new group, in its current place. Its siblings and parent don't change."),
		FNewMenuDelegate::CreateLambda([this, Slot](FMenuBuilder& SubMenu)
		{
			for (const UScriptStruct* ContainerType : EpithDesigner::GetContainerTypes())
			{
				SubMenu.AddMenuEntry(
					ContainerType->GetDisplayNameText(),
					FText::GetEmpty(),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([this, Slot, ContainerType]()
					{
						Edit(FText::Format(LOCTEXT("NestInTransaction", "Nest in {0}"), ContainerType->GetDisplayNameText()), [Slot, ContainerType]()
						{
							EpithDesigner::NestIn(*Slot, ContainerType);
						});
					})));
			}
		}));

	Menu.AddMenuEntry(
		LOCTEXT("Clear", "Clear"),
		LOCTEXT("Clear_Tooltip", "Empty this slot (and everything in it) so you can choose something else."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this, Slot]()
		{
			Edit(LOCTEXT("ClearTransaction", "Clear Slot"), [Slot]()
			{
				Slot->Reset();
			});
		})));

	// Up/down in a Vertical Group; left/right in a Horizontal Group and between tabs
	const bool bUpDown = Actions.Reorder == EEpithSlotReorder::Vertical;

	if (Actions.MoveUp)
	{
		TFunction<void()> MoveUp = Actions.MoveUp;

		Menu.AddMenuEntry(bUpDown ? LOCTEXT("MoveUp", "Move Up") : LOCTEXT("MoveLeft", "Move Left"), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, MoveUp]()
		{
			Edit(LOCTEXT("MoveUpTransaction", "Move Slot"), MoveUp);
		})));
	}

	if (Actions.MoveDown)
	{
		TFunction<void()> MoveDown = Actions.MoveDown;

		Menu.AddMenuEntry(bUpDown ? LOCTEXT("MoveDown", "Move Down") : LOCTEXT("MoveRight", "Move Right"), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, MoveDown]()
		{
			Edit(LOCTEXT("MoveDownTransaction", "Move Slot"), MoveDown);
		})));
	}

	if (Actions.Remove)
	{
		TFunction<void()> Remove = Actions.Remove;

		Menu.AddMenuEntry(
			LOCTEXT("DeleteSlot", "Delete Slot"),
			LOCTEXT("DeleteSlot_Tooltip", "Remove this slot (and everything in it) from its group."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, Remove]()
			{
				Edit(LOCTEXT("DeleteSlotTransaction", "Delete Slot"), Remove);
			})));
	}

	Menu.EndSection();

	return Menu.MakeWidget();
}

// ------------------------------------------------------------------------------------------------

/**
 * Invisible except over a property row's two column lines (Name | Value | Reset), where it shows a resize cursor and a highlight.
 * Dragging a line resizes that column for the whole group live; letting go saves the width to the group's settings.
 * Double-clicking a line sets the column back to automatic. Everywhere else, clicks pass through to the designer frame.
 */
class SEpithColumnHandles : public SLeafWidget
{
public:
	using FOnCommit = TFunction<void(bool bNameColumn, float NewWidth)>;

	SLATE_BEGIN_ARGS(SEpithColumnHandles) {}
		SLATE_ARGUMENT(TSharedPtr<FEpithColumnLayout>, Columns)
		SLATE_ARGUMENT(FOnCommit, OnCommit)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Columns = InArgs._Columns;
		OnCommit = InArgs._OnCommit;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D::ZeroVector;
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const int32 Line = DraggingLine != INDEX_NONE ? DraggingLine : HoveredLine;

		if (Line != INDEX_NONE && Columns.IsValid())
		{
			const float X = GetLineX(Line, AllottedGeometry);

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2f(3.0f, AllottedGeometry.GetLocalSize().Y), FSlateLayoutTransform(FVector2f(X - 1.0f, 0.0f))),
				FAppStyle::GetBrush("WhiteBrush"),
				ESlateDrawEffect::None,
				FStyleColors::Select.GetColor(InWidgetStyle));
		}

		return LayerId + 1;
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		{
			return FReply::Unhandled();
		}

		const float LocalX = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).X;
		const int32 Line = FindLine(LocalX, MyGeometry);

		if (Line == INDEX_NONE)
		{
			return FReply::Unhandled();
		}

		DraggingLine = Line;
		DragStartX = LocalX;
		DragStartWidth = GetColumnWidth(Line, MyGeometry);
		LiveWidth = DragStartWidth;
		bMoved = false;

		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		const float LocalX = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).X;

		if (DraggingLine != INDEX_NONE && HasMouseCapture())
		{
			const float Delta = LocalX - DragStartX;
			bMoved |= FMath::Abs(Delta) >= 1.0f;
			LiveWidth = FMath::RoundToFloat(FMath::Max(MinColumnWidth, DragStartWidth + Delta));

			if (DraggingLine == 0)
			{
				Columns->SetFixedNameWidth(LiveWidth);
			}
			else
			{
				Columns->SetFixedValueWidth(LiveWidth);
			}

			return FReply::Handled();
		}

		HoveredLine = FindLine(LocalX, MyGeometry);
		return FReply::Unhandled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (DraggingLine == INDEX_NONE || MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		{
			return FReply::Unhandled();
		}

		const int32 Line = DraggingLine;
		DraggingLine = INDEX_NONE;

		// A click without a drag changes nothing (and doesn't turn an automatic width into a fixed one)
		if (bMoved && OnCommit)
		{
			OnCommit(Line == 0, LiveWidth);
		}

		return FReply::Handled().ReleaseMouseCapture();
	}

	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		const int32 Line = FindLine(MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).X, MyGeometry);

		if (Line == INDEX_NONE || MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
		{
			return FReply::Unhandled();
		}

		if (OnCommit)
		{
			OnCommit(Line == 0, 0.0f);
		}

		return FReply::Handled();
	}

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override
	{
		SLeafWidget::OnMouseLeave(MouseEvent);
		HoveredLine = INDEX_NONE;
	}

	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override
	{
		DraggingLine = INDEX_NONE;
	}

	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override
	{
		if (DraggingLine != INDEX_NONE || FindLine(MyGeometry.AbsoluteToLocal(CursorEvent.GetScreenSpacePosition()).X, MyGeometry) != INDEX_NONE)
		{
			return FCursorReply::Cursor(EMouseCursor::ResizeLeftRight);
		}

		return FCursorReply::Unhandled();
	}

private:
	static constexpr float GrabDistance = 3.0f;
	static constexpr float MinColumnWidth = 16.0f;

	float GetNameLineX() const
	{
		const FOptionalSize NameWidth = Columns->GetNameWidth();
		return NameWidth.IsSet() ? NameWidth.Get() : 0.0f;
	}

	/** Left edge of a column line, matching the row layout in EpithPropertyRow: the value column fills the row unless its width is fixed. */
	float GetLineX(int32 Line, const FGeometry& Geometry) const
	{
		const float NameX = GetNameLineX();

		if (Line == 0)
		{
			return NameX;
		}

		const FOptionalSize ValueWidth = Columns->GetValueWidth();

		return ValueWidth.IsSet()
			? NameX + FEpithColumnLayout::LineWidth + ValueWidth.Get()
			: Geometry.GetLocalSize().X - FEpithColumnLayout::ResetColumnWidth - FEpithColumnLayout::LineWidth;
	}

	float GetColumnWidth(int32 Line, const FGeometry& Geometry) const
	{
		return Line == 0 ? GetNameLineX() : GetLineX(1, Geometry) - GetNameLineX() - FEpithColumnLayout::LineWidth;
	}

	int32 FindLine(float LocalX, const FGeometry& Geometry) const
	{
		if (!Columns.IsValid())
		{
			return INDEX_NONE;
		}

		int32 Best = INDEX_NONE;
		float BestDistance = GrabDistance;

		for (int32 Line = 0; Line < 2; ++Line)
		{
			const float Distance = FMath::Abs(LocalX - (GetLineX(Line, Geometry) + 0.5f));

			if (Distance <= BestDistance)
			{
				Best = Line;
				BestDistance = Distance;
			}
		}

		return Best;
	}

	TSharedPtr<FEpithColumnLayout> Columns;
	FOnCommit OnCommit;

	int32 HoveredLine = INDEX_NONE;
	int32 DraggingLine = INDEX_NONE;
	float DragStartX = 0.0f;
	float DragStartWidth = 0.0f;
	float LiveWidth = 0.0f;
	bool bMoved = false;
};

TSharedRef<SWidget> SEpithPanelDesigner::MakeInsertDivider(bool bRunsHorizontally, const FText& ToolTip, TFunction<void()> InsertSlot)
{
	constexpr float ButtonThickness = 3.0f;
	constexpr float LineThickness = 1.0f;

	TSharedRef<TWeakPtr<SWidget>> ButtonHolder = MakeShared<TWeakPtr<SWidget>>();

	// A 1px line, faint until hovered
	TSharedRef<SWidget> Line = SNew(SBox)
		.WidthOverride(bRunsHorizontally ? FOptionalSize() : FOptionalSize(LineThickness))
		.HeightOverride(bRunsHorizontally ? FOptionalSize(LineThickness) : FOptionalSize())
		[
			SNew(SImage)
			.Image(FAppStyle::GetBrush("WhiteBrush"))
			.ColorAndOpacity_Lambda([ButtonHolder]()
			{
				TSharedPtr<SWidget> Button = ButtonHolder->Pin();

				return Button.IsValid() && Button->IsHovered()
					? FSlateColor(EStyleColor::Select)
					: FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.12f));
			})
		];

	// The 3px button around it (1px of padding either side of the line)
	TSharedRef<SButton> Button = SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "NoBorder")
		.ContentPadding(bRunsHorizontally ? FMargin(0.0f, 1.0f) : FMargin(1.0f, 0.0f))
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Cursor(EMouseCursor::Hand)
		.ToolTipText(ToolTip)
		.OnClicked_Lambda([this, InsertSlot]()
		{
			Edit(LOCTEXT("InsertSlotTransaction", "Insert Slot"), InsertSlot);
			return FReply::Handled();
		})
		[
			Line
		];

	*ButtonHolder = Button;

	return SNew(SBox)
		.WidthOverride(bRunsHorizontally ? FOptionalSize() : FOptionalSize(ButtonThickness))
		.HeightOverride(bRunsHorizontally ? FOptionalSize(ButtonThickness) : FOptionalSize())
		[
			Button
		];
}

TSharedRef<SWidget> SEpithPanelDesigner::MakeControlPicker(TFunction<void(const UScriptStruct*)> OnPicked)
{
	return SNew(SComboButton)
		.ComboButtonStyle(&EpithDesigner::GetChooseComboStyle())
		.ContentPadding(FMargin(4, 1))
		.OnGetMenuContent_Lambda([this, OnPicked]()
		{
			FMenuBuilder Menu(true, nullptr);
			const TArray<const UScriptStruct*> Types = EpithDesigner::GetControlTypes();

			if (Types.Num() == 0)
			{
				Menu.AddWidget(SNew(STextBlock).Text(LOCTEXT("ControlPicker_None", "No special control types found.")), FText::GetEmpty());
			}

			for (const UScriptStruct* Type : Types)
			{
				Menu.AddMenuEntry(
					Type->GetDisplayNameText(),
					Type->GetToolTipText(),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([this, OnPicked, Type]()
					{
						Edit(FText::Format(LOCTEXT("SetControl", "Set Control: {0}"), Type->GetDisplayNameText()), [OnPicked, Type]()
						{
							OnPicked(Type);
						});
					})));
			}

			return Menu.MakeWidget();
		})
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ControlPicker_Choose", "Pick control..."))
		];
}

TSharedRef<SWidget> SEpithPanelDesigner::MakeColumnHandles(const TSharedRef<FEpithColumnLayout>& Columns)
{
	// The group's settings, inside the definition being edited (everything is rebuilt on each redraw, so this stays valid while shown)
	FEpithColumnSettings* Settings = const_cast<FEpithColumnSettings*>(Columns->GetSourceSettings());

	if (!Settings)
	{
		return SNullWidget::NullWidget;
	}

	return SNew(SEpithColumnHandles)
		.Columns(Columns)
		.OnCommit([this, Settings](bool bNameColumn, float NewWidth)
		{
			const FText Description = bNameColumn
				? LOCTEXT("ResizeNameColumn", "Resize Name Column")
				: LOCTEXT("ResizeValueColumn", "Resize Value Column");

			Edit(Description, [Settings, bNameColumn, NewWidth]()
			{
				(bNameColumn ? Settings->NameColumnWidth : Settings->ValueColumnWidth) = NewWidth;
			});
		});
}

#undef LOCTEXT_NAMESPACE
