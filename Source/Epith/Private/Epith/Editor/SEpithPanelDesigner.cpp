#include "SEpithPanelDesigner.h"

#include "EpithPropertyMenu.h"
#include "ISinglePropertyView.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Editor.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"
#include "Epith/Style/EpithStyle.h"
#include "Epith/Widgets/EpithWindowElement_FoldoutGroup.h"
#include "Epith/Widgets/EpithWindowElement_HorizontalGroup.h"
#include "Epith/Widgets/EpithWindowElement_Property.h"
#include "Epith/Widgets/EpithWindowElement_TabbedGroup.h"
#include "Epith/Widgets/EpithWindowElement_VerticalGroup.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Widgets/SBoxPanel.h"
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
			FEpithWindowElement_FoldoutGroup::StaticStruct(),
			FEpithWindowElement_VerticalGroup::StaticStruct(),
			FEpithWindowElement_HorizontalGroup::StaticStruct(),
			FEpithWindowElement_TabbedGroup::StaticStruct(),
			FEpithWindowElement_Property::StaticStruct(),
		};

		return Types;
	}

	/** Element types that can hold other elements (for "Nest in"). */
	const TArray<const UScriptStruct*>& GetContainerTypes()
	{
		static const TArray<const UScriptStruct*> Types =
		{
			FEpithWindowElement_FoldoutGroup::StaticStruct(),
			FEpithWindowElement_VerticalGroup::StaticStruct(),
			FEpithWindowElement_HorizontalGroup::StaticStruct(),
			FEpithWindowElement_TabbedGroup::StaticStruct(),
		};

		return Types;
	}

	/** Turns a slot into a new element of Type, with a first empty child slot for groups so there's something to click. */
	void InitializeElement(TInstancedStruct<FEpithWindowElement>& Slot, const UScriptStruct* Type)
	{
		Slot.InitializeAsScriptStruct(Type);

		if (FEpithWindowElement_VerticalGroup* Vertical = Slot.GetMutablePtr<FEpithWindowElement_VerticalGroup>())
		{
			Vertical->Slots.AddDefaulted();
		}
		else if (FEpithWindowElement_HorizontalGroup* Horizontal = Slot.GetMutablePtr<FEpithWindowElement_HorizontalGroup>())
		{
			Horizontal->Slots.AddDefaulted();
		}
		else if (FEpithWindowElement_TabbedGroup* Tabbed = Slot.GetMutablePtr<FEpithWindowElement_TabbedGroup>())
		{
			Tabbed->Slots.AddDefaulted_GetRef().TabName = TEXT("Tab 1");
		}
		else if (FEpithWindowElement_FoldoutGroup* Foldout = Slot.GetMutablePtr<FEpithWindowElement_FoldoutGroup>())
		{
			Foldout->FoldoutTitle = TEXT("Foldout");
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
		else if (FEpithWindowElement_FoldoutGroup* Foldout = Slot.GetMutablePtr<FEpithWindowElement_FoldoutGroup>())
		{
			Foldout->FoldoutTitle = TEXT("Foldout");
			Foldout->Element = MoveTemp(Inner);
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
		, _Padding(FMargin(2))
		{}

		SLATE_ARGUMENT(bool, bDashed)
		SLATE_ARGUMENT(bool, bOnlyWhenHovered)

		/** Outline the content rather than the whole frame, so the padding stays outside the outline. */
		SLATE_ARGUMENT(bool, bOutlineContent)
		SLATE_ARGUMENT(FMargin, Padding)
		SLATE_EVENT(FOnGetContent, OnGetContextMenu)
		SLATE_DEFAULT_SLOT(FArguments, Content)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		bDashed = InArgs._bDashed;
		bOnlyWhenHovered = InArgs._bOnlyWhenHovered;
		bOutlineContent = InArgs._bOutlineContent;
		OnGetContextMenu = InArgs._OnGetContextMenu;

		ChildSlot
		.Padding(InArgs._Padding)
		[
			InArgs._Content.Widget
		];
	}

	int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const int32 MaxLayer = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

		if (bOnlyWhenHovered && !IsHovered())
		{
			return MaxLayer;
		}

		const FSlateBrush* Brush = FAppStyle::GetBrush("WhiteBrush");
		const FLinearColor Color = bDashed
			? FSlateColor(EStyleColor::Foreground).GetColor(InWidgetStyle).CopyWithNewOpacity(0.4f)
			: FSlateColor(EStyleColor::Primary).GetColor(InWidgetStyle);

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

		constexpr float Thickness = 1.0f;

		if (!bDashed)
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
	FOnGetContent OnGetContextMenu;
};

// ================================================================================================
// SEpithPanelDesigner
// ================================================================================================

void SEpithPanelDesigner::Construct(const FArguments& InArgs, UEpithPanelDefinition* InDefinition)
{
	Definition = InDefinition;
	OnDefinitionEdited = InArgs._OnDefinitionEdited;

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
	PreviewData.Reset();

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

		return;
	}

	// The preview shows the class defaults: values don't matter here, only the layout
	PreviewData = MakeShared<FEpithPropertyDataContainer>(TargetClass->GetDefaultObject());

	const FEpithDrawContext Context(PreviewData, SharedThis(this));

	TSharedPtr<SWidget> Content = EpithDraw::DrawSlot(PanelDefinition->GetRootForEditing(), Context);

	const FText Title = PanelDefinition->GetTitle().IsEmpty() ? TargetClass->GetDisplayNameText() : PanelDefinition->GetTitle();

	// Styled like the object pane in the viewport, so what you build is what you get
	PreviewArea->SetContent(
		SNew(SBox)
		.HAlign(HAlign_Left)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Menu.Background"))
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
}

void SEpithPanelDesigner::Edit(const FText& Description, TFunction<void()> Mutation)
{
	UEpithPanelDefinition* PanelDefinition = Definition.Get();

	if (!PanelDefinition || !Mutation)
	{
		return;
	}

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
	// Covers the Target Class picker, the Details tab and our own edits
	if (Object && Object == Definition.Get())
	{
		RequestRedraw();
	}
}

void SEpithPanelDesigner::PostUndo(bool bSuccess)
{
	OnDefinitionEdited.ExecuteIfBound();
	RequestRedraw();
}

void SEpithPanelDesigner::PostRedo(bool bSuccess)
{
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
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SComboButton)
			.ContentPadding(FMargin(6, 2))
			.OnGetMenuContent_Lambda([this, SlotPtr]()
			{
				return MakeElementTypeMenu(SlotPtr);
			})
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("EmptySlot_Choose", "Choose element..."))
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
			.ToolTipText(LOCTEXT("EmptySlot_Remove", "Remove this slot"))
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

	return SNew(SEpithDesignerFrame)
		.bDashed(true)
		.Padding(FMargin(8, 6))
		[
			Row
		];
}

TSharedRef<SWidget> SEpithPanelDesigner::WrapElement(TInstancedStruct<FEpithWindowElement>& Slot, TSharedRef<SWidget> ElementWidget, const FEpithSlotActions& Actions)
{
	TInstancedStruct<FEpithWindowElement>* SlotPtr = &Slot;

	const UScriptStruct* Type = Slot.GetScriptStruct();
	const FText TypeName = Type ? Type->GetDisplayNameText() : FText::GetEmpty();

	// The menu button shows while the frame is hovered; the frame is created after the button, so share a pointer to it
	TSharedRef<TWeakPtr<SWidget>> FrameHolder = MakeShared<TWeakPtr<SWidget>>();

	TSharedRef<SWidget> MenuButton = SNew(SComboButton)
		.HasDownArrow(false)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ContentPadding(FMargin(4, 0))
		.ToolTipText(FText::Format(LOCTEXT("Element_MenuTooltip", "{0} options (or right-click)"), TypeName))
		.Visibility_Lambda([FrameHolder]()
		{
			TSharedPtr<SWidget> Frame = FrameHolder->Pin();
			return Frame.IsValid() && Frame->IsHovered() ? EVisibility::Visible : EVisibility::Hidden;
		})
		.OnGetMenuContent_Lambda([this, SlotPtr, Actions]()
		{
			return MakeElementMenu(SlotPtr, Actions);
		})
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("Element_MenuButton", "..."))
		];

	TSharedRef<SEpithDesignerFrame> Frame = SNew(SEpithDesignerFrame)
		.bOnlyWhenHovered(true)
		.bOutlineContent(true)
		// Each nesting level is inset by this much, so a child's "..." button never sits on top of its parent's
		.Padding(FMargin(14))
		.OnGetContextMenu_Lambda([this, SlotPtr, Actions]()
		{
			return MakeElementMenu(SlotPtr, Actions);
		})
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				ElementWidget
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Top)
			[
				MenuButton
			]
		];

	*FrameHolder = Frame;

	return Frame;
}

TSharedRef<SWidget> SEpithPanelDesigner::MakeAddButton(const FText& Label, TFunction<void()> AddSlot)
{
	return SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ContentPadding(FMargin(4, 1))
		.OnClicked_Lambda([this, Label, AddSlot]()
		{
			Edit(Label, AddSlot);
			return FReply::Handled();
		})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.Plus"))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4, 0, 0, 0)
			[
				SNew(STextBlock)
				.Text(Label)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
}

TSharedRef<SWidget> SEpithPanelDesigner::MakePropertyPicker(FName CurrentPath, TFunction<void(FName)> OnPicked)
{
	return SNew(SComboButton)
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
			});
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

TSharedRef<SWidget> SEpithPanelDesigner::MakeElementTypeMenu(TInstancedStruct<FEpithWindowElement>* Slot)
{
	FMenuBuilder Menu(true, nullptr);

	for (const UScriptStruct* Type : EpithDesigner::GetElementTypes())
	{
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

	Menu.BeginSection(NAME_None, Type ? Type->GetDisplayNameText() : FText::GetEmpty());

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

	if (Actions.MoveUp)
	{
		TFunction<void()> MoveUp = Actions.MoveUp;

		Menu.AddMenuEntry(LOCTEXT("MoveUp", "Move Up"), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, MoveUp]()
		{
			Edit(LOCTEXT("MoveUpTransaction", "Move Element"), MoveUp);
		})));
	}

	if (Actions.MoveDown)
	{
		TFunction<void()> MoveDown = Actions.MoveDown;

		Menu.AddMenuEntry(LOCTEXT("MoveDown", "Move Down"), FText::GetEmpty(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, MoveDown]()
		{
			Edit(LOCTEXT("MoveDownTransaction", "Move Element"), MoveDown);
		})));
	}

	if (Actions.Remove)
	{
		TFunction<void()> Remove = Actions.Remove;

		Menu.AddMenuEntry(
			LOCTEXT("Remove", "Remove from Group"),
			LOCTEXT("Remove_Tooltip", "Remove this slot (and everything in it) from its group."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, Remove]()
			{
				Edit(LOCTEXT("RemoveTransaction", "Remove Element"), Remove);
			})));
	}

	Menu.EndSection();

	return Menu.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
