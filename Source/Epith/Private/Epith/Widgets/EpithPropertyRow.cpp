#include "EpithPropertyRow.h"

#include "DetailTreeNode.h"
#include "DetailWidgetRow.h"
#include "IDetailTreeNode.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"
#include "Epith/Widgets/EpithDrawContext.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/StyleColors.h"
#include "UObject/UnrealType.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "Epith"

namespace EpithPropertyRow
{
	// Same measurements as the Details panel (SDetailSingleItemRow, SDetailRowIndent, PropertyEditorConstants)
	constexpr float IndentPerLevel = 16.0f;
	constexpr float ExpanderWidth = 16.0f;
	constexpr float RowMinHeight = 26.0f;
	constexpr float ResetColumnWidth = FEpithColumnLayout::ResetColumnWidth;

	/** Space above and below each row's widgets, so tall values don't touch the lines. */
	constexpr float RowVerticalPadding = 2.0f;

	/**
	 * The Details panel's row color (PropertyEditorConstants::GetRowBackgroundColor, which is private to PropertyEditor):
	 * the panel color, a few shades lighter per nesting level (bouncing back after 3 levels), and the header color while hovered.
	 */
	FSlateColor GetRowBackgroundColor(int32 Level, bool bHovered)
	{
		int32 ColorIndex = 0;
		int32 Increment = 1;

		for (int32 i = 0; i < Level; ++i)
		{
			ColorIndex += Increment;

			if (ColorIndex == 0 || ColorIndex == 3)
			{
				Increment = -Increment;
			}
		}

		static const uint8 ColorOffsets[] = { 0, 4, 4 + 2, 6 + 4, 10 + 6 };

		const FSlateColor BaseSlateColor = FAppStyle::Get().GetSlateColor(bHovered ? "Colors.Header" : "Colors.Panel");
		const FColor BaseColor = BaseSlateColor.GetSpecifiedColor().ToFColor(true);
		const uint8 Offset = ColorOffsets[FMath::Clamp(ColorIndex, 0, 4)];

		return FSlateColor(FLinearColor::FromSRGBColor(FColor(BaseColor.R + Offset, BaseColor.G + Offset, BaseColor.B + Offset)));
	}

	/** One strip per nesting level in that level's color, with the array drop shadow, like SDetailRowIndent. */
	class SEpithRowIndent : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SEpithRowIndent) {}
			SLATE_ARGUMENT(int32, Depth)
			SLATE_ATTRIBUTE(bool, IsRowHovered)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Depth = InArgs._Depth;
			IsRowHovered = InArgs._IsRowHovered;
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(Depth * IndentPerLevel, 0.0f);
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			const FSlateBrush* BackgroundBrush = FAppStyle::Get().GetBrush("DetailsView.CategoryMiddle");
			const FSlateBrush* DropShadowBrush = FAppStyle::Get().GetBrush("DetailsView.ArrayDropShadow");
			const bool bHovered = IsRowHovered.Get(false);
			const float Height = AllottedGeometry.GetLocalSize().Y;

			for (int32 i = 0; i < Depth; ++i)
			{
				const FPaintGeometry Strip = AllottedGeometry.ToPaintGeometry(FVector2f(IndentPerLevel, Height), FSlateLayoutTransform(FVector2f(IndentPerLevel * i, 0.0f)));

				FSlateDrawElement::MakeBox(OutDrawElements, LayerId, Strip, BackgroundBrush, ESlateDrawEffect::None, GetRowBackgroundColor(i, bHovered).GetColor(InWidgetStyle));
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1, Strip, DropShadowBrush);
			}

			return LayerId + 1;
		}

	private:
		int32 Depth = 0;
		TAttribute<bool> IsRowHovered;
	};

	/** A 1px column divider; the gaps between the Details panel's splitter columns. */
	TSharedRef<SWidget> MakeColumnLine()
	{
		return SNew(SBox)
			.WidthOverride(FEpithColumnLayout::LineWidth)
			[
				SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("DetailsView.GridLine"))
			];
	}

	/**
	 * Every IDetailTreeNode a property row generator hands out is an FDetailTreeNode (the engine relies on this as well, in
	 * FPropertyRowGenerator::FindTreeNodeRecursive). Its virtual GenerateStandaloneWidget() gives us the whole FDetailWidgetRow,
	 * including the edit condition, enabled state and reset-to-default override that IDetailTreeNode::CreateNodeWidgets() drops.
	 * GenerateStandaloneWidget() reuses the row built when the node was created, so it does not re-run type customizations.
	 */
	FDetailTreeNode& AsDetailTreeNode(IDetailTreeNode& Node)
	{
		return static_cast<FDetailTreeNode&>(Node);
	}

	/** Key used to remember expansion and to stop the same row being drawn twice. */
	FName GetNodeKey(const TSharedRef<IDetailTreeNode>& Node, const TSharedPtr<IPropertyHandle>& Handle)
	{
		if (Handle.IsValid() && Handle->IsValidHandle())
		{
			return FName(Handle->GeneratePathToProperty());
		}

		return Node->GetNodeName();
	}

	/** The children to draw under a node. Nodes that only group other rows (categories, advanced sections, header-less builders) are replaced by their children. */
	void GatherDrawableChildren(const TSharedRef<IDetailTreeNode>& Node, TArray<TSharedRef<IDetailTreeNode>>& OutChildren)
	{
		TArray<TSharedRef<IDetailTreeNode>> Children;

		// Include rows that are hidden right now; each row binds its own visibility, so it appears as soon as it should.
		Node->GetChildren(Children, true);

		for (const TSharedRef<IDetailTreeNode>& Child : Children)
		{
			const bool bIsGroupingNode = Child->GetNodeType() != EDetailNodeType::Item || AsDetailTreeNode(*Child).ShouldShowOnlyChildren();

			if (bIsGroupingNode)
			{
				GatherDrawableChildren(Child, OutChildren);
			}
			else
			{
				OutChildren.Add(Child);
			}
		}
	}

	/** Collapsed whenever the details system hides the row: EditConditionHides, a customization's Visibility, HideProperty, ... */
	TAttribute<EVisibility> MakeNodeVisibility(const TSharedRef<IDetailTreeNode>& Node)
	{
		TWeakPtr<IDetailTreeNode> WeakNode = Node;

		return TAttribute<EVisibility>::CreateLambda([WeakNode]()
		{
			TSharedPtr<IDetailTreeNode> PinnedNode = WeakNode.Pin();

			if (!PinnedNode.IsValid())
			{
				return EVisibility::Visible;
			}

			return AsDetailTreeNode(*PinnedNode).GetVisibility() == ENodeVisibility::ForcedHidden ? EVisibility::Collapsed : EVisibility::Visible;
		});
	}

	/** The checkbox for properties with an inline edit condition (InlineEditConditionToggle). */
	TSharedRef<SWidget> MakeEditConditionToggle(const FDetailWidgetRow& Row)
	{
		TAttribute<bool> EditConditionValue = Row.EditConditionValue;
		FOnBooleanValueChanged OnEditConditionValueChanged = Row.OnEditConditionValueChanged;

		return SNew(SCheckBox)
			.IsChecked_Lambda([EditConditionValue]()
			{
				return EditConditionValue.Get(true) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			})
			.OnCheckStateChanged_Lambda([OnEditConditionValueChanged](ECheckBoxState NewState)
			{
				FScopedTransaction Transaction(LOCTEXT("PropertyRow_EditConditionChanged", "Edit Condition Changed"));
				OnEditConditionValueChanged.ExecuteIfBound(NewState == ECheckBoxState::Checked);
			});
	}

	/** The yellow reset arrow. Same rules as the Details panel: a custom override wins, NoResetToDefault hides it, otherwise CanResetToDefault(). */
	TSharedRef<SWidget> MakeResetToDefaultButton(const TSharedRef<IPropertyHandle>& Handle, const FDetailWidgetRow& Row)
	{
		TOptional<FResetToDefaultOverride> ResetOverride = Row.CustomResetToDefault;
		TAttribute<bool> EditConditionValue = Row.EditConditionValue;
		TAttribute<bool> RowIsEnabled = Row.IsEnabledAttr;

		const bool bNoResetToDefault = Handle->HasMetaData(TEXT("NoResetToDefault")) || Handle->GetInstanceMetaData(TEXT("NoResetToDefault")) != nullptr;

		auto IsResetVisible = [Handle, ResetOverride, bNoResetToDefault]() -> bool
		{
			if (!Handle->IsValidHandle())
			{
				return false;
			}

			if (ResetOverride.IsSet())
			{
				return ResetOverride.GetValue().IsResetToDefaultVisible(Handle);
			}

			return !bNoResetToDefault && Handle->CanResetToDefault();
		};

		return SNew(SBox)
			.WidthOverride(ExpanderWidth)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ContentPadding(0)
				.ToolTipText(LOCTEXT("PropertyRow_ResetToDefault", "Reset this property to its default value."))
				.Visibility_Lambda([IsResetVisible]()
				{
					return IsResetVisible() ? EVisibility::Visible : EVisibility::Hidden;
				})
				.IsEnabled_Lambda([Handle, EditConditionValue, RowIsEnabled]()
				{
					return EditConditionValue.Get(true) && RowIsEnabled.Get(true) && Handle->IsValidHandle() && Handle->IsEditable();
				})
				.OnClicked_Lambda([Handle, ResetOverride]()
				{
					if (ResetOverride.IsSet())
					{
						ResetOverride.GetValue().OnResetToDefaultClicked(Handle);
					}
					else
					{
						Handle->ResetToDefault();
					}

					return FReply::Handled();
				})
				[
					SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("PropertyWindow.DiffersFromDefault"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			];
	}

	/**
	 * EditDefaultsOnly properties (and Blueprint variables that aren't Instance Editable) are included in the rows,
	 * but aren't meant to be changed on placed instances. Show them read-only, the way the class defaults would look.
	 */
	TSharedRef<SWidget> WrapIfNotInstanceEditable(TSharedRef<SWidget> RowWidget, const TSharedPtr<IPropertyHandle>& Handle, const FEpithPropertyDataContainer& PropertyData)
	{
		const FProperty* EditBlocker = FEpithPropertyDataContainer::FindInstanceEditBlocker(Handle);

		if (!EditBlocker || !PropertyData.IsEditingInstance())
		{
			return RowWidget;
		}

		const bool bIsBlueprintVariable = Cast<UBlueprintGeneratedClass>(EditBlocker->GetOwnerClass()) != nullptr;

		const FText Reason = bIsBlueprintVariable
			? FText::Format(LOCTEXT("PropertyRow_ReadOnlyBlueprint", "Read-only: {0} isn't Instance Editable. To edit it here, open the Blueprint and click the eye icon next to the variable."), EditBlocker->GetDisplayNameText())
			: FText::Format(LOCTEXT("PropertyRow_ReadOnlyDefaults", "Read-only: {0} is EditDefaultsOnly, so it can only be changed on the class defaults."), EditBlocker->GetDisplayNameText());

		return SNew(SBox)
			.IsEnabled(false)
			.ToolTipText(Reason)
			[
				RowWidget
			];
	}

	TSharedRef<SWidget> MakeWarning(const FText& Message, int32 Depth)
	{
		return SNew(SBox)
			.Padding(FMargin(Depth * IndentPerLevel + (Depth > 0 ? ExpanderWidth : 0.0f), 0, 0, 0))
			[
				SNew(STextBlock)
				.Text(Message)
				.ColorAndOpacity(FSlateColor(EStyleColor::Warning))
			];
	}

	TSharedRef<SWidget> MakeChildRows(const TArray<TSharedRef<IDetailTreeNode>>& Children, const TSharedRef<FEpithPropertyDataContainer>& PropertyData, int32 Depth, const TSharedPtr<FEpithColumnLayout>& Columns)
	{
		TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

		for (const TSharedRef<IDetailTreeNode>& Child : Children)
		{
			const FName ChildKey = GetNodeKey(Child, Child->CreatePropertyHandle());

			TSharedRef<SWidget> ChildWidget = PropertyData->TryClaimChildPath(ChildKey)
				? Make(Child, PropertyData, Depth, Columns)
				: MakeWarning(FText::Format(LOCTEXT("PropertyRow_ChildDuplicate", "{0} is already shown in this panel"), FText::FromName(ChildKey)), Depth);

			Box->AddSlot()
			.AutoHeight()
			.Padding(0, 0)
			[
				ChildWidget
			];
		}

		return Box;
	}

	/**
	 * The child list of one expandable row. Built the first time it's shown (big arrays cost nothing while collapsed), and
	 * redrawn on its own when the row generator refreshes and this row's children actually changed (e.g. an array element
	 * was added). Nothing else in the panel is rebuilt.
	 */
	struct FChildListState
	{
		TWeakPtr<IDetailTreeNode> Node;
		TWeakPtr<FEpithPropertyDataContainer> PropertyData;
		TWeakPtr<SBox> Container;
		FName Key;
		int32 Depth = 0;
		TSharedPtr<FEpithColumnLayout> Columns;
		bool bBuilt = false;
		TArray<TWeakPtr<IDetailTreeNode>> BuiltChildren;

		void Build()
		{
			TSharedPtr<IDetailTreeNode> PinnedNode = Node.Pin();
			TSharedPtr<FEpithPropertyDataContainer> PinnedData = PropertyData.Pin();
			TSharedPtr<SBox> PinnedContainer = Container.Pin();

			if (!PinnedNode.IsValid() || !PinnedData.IsValid() || !PinnedContainer.IsValid())
			{
				return;
			}

			// Let go of the old child widgets and their claims before drawing the new ones
			PinnedContainer->SetContent(SNullWidget::NullWidget);
			PinnedData->ReleaseChildClaimsUnder(Key);

			TArray<TSharedRef<IDetailTreeNode>> Children;
			GatherDrawableChildren(PinnedNode.ToSharedRef(), Children);

			BuiltChildren.Reset(Children.Num());

			for (const TSharedRef<IDetailTreeNode>& Child : Children)
			{
				BuiltChildren.Add(Child);
			}

			PinnedContainer->SetContent(MakeChildRows(Children, PinnedData.ToSharedRef(), Depth + 1, Columns));
			bBuilt = true;

			// New child rows may be wider than the group's column; measure them now so the column doesn't jump a frame later
			if (Columns.IsValid())
			{
				FEpithColumnLayout::MeasureNow(PinnedContainer.ToSharedRef());
			}
		}

		void BuildIfNeeded()
		{
			if (!bBuilt)
			{
				Build();
			}
		}

		bool HaveChildrenChanged() const
		{
			TSharedPtr<IDetailTreeNode> PinnedNode = Node.Pin();

			if (!PinnedNode.IsValid())
			{
				return false;
			}

			TArray<TSharedRef<IDetailTreeNode>> Children;
			GatherDrawableChildren(PinnedNode.ToSharedRef(), Children);

			if (Children.Num() != BuiltChildren.Num())
			{
				return true;
			}

			for (int32 i = 0; i < Children.Num(); ++i)
			{
				if (BuiltChildren[i].Pin() != Children[i])
				{
					return true;
				}
			}

			return false;
		}
	};

	TSharedRef<SWidget> Make(TSharedRef<IDetailTreeNode> Node, TSharedRef<FEpithPropertyDataContainer> PropertyData, int32 Depth, TSharedPtr<FEpithColumnLayout> Columns)
	{
		FDetailTreeNode& DetailNode = AsDetailTreeNode(*Node);
		TSharedPtr<IPropertyHandle> Handle = Node->CreatePropertyHandle();
		const FName Key = GetNodeKey(Node, Handle);

		TArray<TSharedRef<IDetailTreeNode>> Children;
		GatherDrawableChildren(Node, Children);

		// Arrays, sets and maps can gain children later (an empty array gets its first element), so they always get an expander
		const FProperty* RowProperty = Handle.IsValid() && Handle->IsValidHandle() ? Handle->GetProperty() : nullptr;
		const bool bIsContainerProperty = RowProperty && (RowProperty->IsA<FArrayProperty>() || RowProperty->IsA<FSetProperty>() || RowProperty->IsA<FMapProperty>());
		const bool bCanHaveChildren = Children.Num() > 0 || bIsContainerProperty;

		// Some rows have no header of their own and only show their children (e.g. a struct with ShowOnlyInnerProperties)
		if (DetailNode.ShouldShowOnlyChildren())
		{
			return SNew(SBox)
				.Visibility(MakeNodeVisibility(Node))
				[
					WrapIfNotInstanceEditable(MakeChildRows(Children, PropertyData, Depth, Columns), Handle, *PropertyData)
				];
		}

		FDetailWidgetRow Row;
		const bool bHasRowContent = DetailNode.GenerateStandaloneWidget(Row) && Row.HasAnyContent();

		// Everything left of the value: indent, expander, edit-condition toggle, name. In a group this whole area gets the
		// group's shared name-column width, so values line up even for nested (indented) rows, like the Details panel.
		TSharedRef<SHorizontalBox> NameArea = SNew(SHorizontalBox);

		// Whether this row's own line (not its children) is under the mouse; set once the row's border exists
		TSharedRef<TWeakPtr<SWidget>> HoverSource = MakeShared<TWeakPtr<SWidget>>();
		const TAttribute<bool> IsRowHovered = TAttribute<bool>::CreateLambda([HoverSource]()
		{
			TSharedPtr<SWidget> Source = HoverSource->Pin();
			return Source.IsValid() && Source->IsHovered();
		});

		if (Depth > 0)
		{
			NameArea->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Fill)
			[
				SNew(SEpithRowIndent)
				.Depth(Depth)
				.IsRowHovered(IsRowHovered)
			];
		}

		// Expander arrow for structs, arrays, maps, sets and customized children; a spacer keeps nested rows aligned
		TSharedPtr<SBox> ChildContainer;

		if (bCanHaveChildren)
		{
			TWeakPtr<FEpithPropertyDataContainer> WeakPropertyData = PropertyData;

			ChildContainer = SNew(SBox)
				.Visibility_Lambda([WeakPropertyData, Key]()
				{
					TSharedPtr<FEpithPropertyDataContainer> PinnedData = WeakPropertyData.Pin();
					return PinnedData.IsValid() && PinnedData->IsExpanded(Key) ? EVisibility::Visible : EVisibility::Collapsed;
				});

			TSharedRef<FChildListState> ChildList = MakeShared<FChildListState>();
			ChildList->Node = Node;
			ChildList->PropertyData = PropertyData;
			ChildList->Container = ChildContainer;
			ChildList->Key = Key;
			ChildList->Depth = Depth;
			ChildList->Columns = Columns;

			if (PropertyData->IsExpanded(Key))
			{
				ChildList->Build();
			}

			// When the generator refreshes (e.g. an element was added to this array), redraw just this row's children if they changed.
			// Bound to the child container's lifetime, so it unbinds itself when the row goes away.
			PropertyData->OnRowsRefreshed.AddSPLambda(ChildContainer.Get(), [ChildList]()
			{
				if (ChildList->bBuilt && ChildList->HaveChildrenChanged())
				{
					ChildList->Build();
				}
			});

			NameArea->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(ExpanderWidth)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FCoreStyle::Get(), "NoBorder")
					.ClickMethod(EButtonClickMethod::MouseDown)
					.ContentPadding(0)
					.OnClicked_Lambda([WeakPropertyData, Key, ChildList]()
					{
						if (TSharedPtr<FEpithPropertyDataContainer> PinnedData = WeakPropertyData.Pin())
						{
							const bool bNewExpanded = !PinnedData->IsExpanded(Key);
							PinnedData->SetExpanded(Key, bNewExpanded);

							if (bNewExpanded)
							{
								ChildList->BuildIfNeeded();
							}
						}

						return FReply::Handled();
					})
					[
						SNew(SImage)
						.ColorAndOpacity(FSlateColor::UseForeground())
						.Image_Lambda([WeakPropertyData, Key]()
						{
							TSharedPtr<FEpithPropertyDataContainer> PinnedData = WeakPropertyData.Pin();
							const bool bExpanded = PinnedData.IsValid() && PinnedData->IsExpanded(Key);
							return FAppStyle::Get().GetBrush(bExpanded ? "TreeArrow_Expanded" : "TreeArrow_Collapsed");
						})
					]
				]
			];
		}
		else
		{
			// Rows without children keep the arrow's space, so names line up (the Details panel does the same)
			NameArea->AddSlot()
			.AutoWidth()
			[
				SNew(SSpacer)
				.Size(FVector2D(ExpanderWidth, 1))
			];
		}

		if (Row.OnEditConditionValueChanged.IsBound())
		{
			NameArea->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 4, 0)
			[
				MakeEditConditionToggle(Row)
			];
		}

		TSharedPtr<SWidget> ValueWidget;

		if (!bHasRowContent)
		{
			// Nothing to show for the row itself (rare); fall back to the property's name so its children still have a header
			NameArea->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Handle.IsValid() ? Handle->GetPropertyDisplayName() : FText::FromName(Key))
			];
		}
		else if (Row.HasColumns())
		{
			NameArea->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2, RowVerticalPadding, 10, RowVerticalPadding)
			[
				Row.NameWidget.Widget
			];

			// The value's own minimum width, as the Details panel applies it. Its maximum isn't applied: the value stretches to
			// fill the value column, whose width the panel's author sets.
			const TOptional<float> MinWidth = Row.ValueWidget.MinWidth;

			ValueWidget = SNew(SBox)
				.MinDesiredWidth(MinWidth.IsSet() ? FOptionalSize(MinWidth.GetValue()) : FOptionalSize())
				[
					Row.ValueWidget.Widget
				];
		}
		else
		{
			// Rows that draw themselves as one piece (some customizations) keep their own layout
			NameArea->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				Row.WholeRowWidget.Widget
			];
		}

		// | Name | Value | Reset |, laid out like a Details panel row: 1px lines between the columns, and the row background
		// covers all but the bottom pixel of a grid-line colored border, which shows through as the line under the row.
		TSharedRef<SHorizontalBox> RowBox = SNew(SHorizontalBox);
		const bool bAligned = Columns.IsValid() && ValueWidget.IsValid();

		if (bAligned)
		{
			Columns->RegisterNameArea(NameArea);
		}

		// Name
		RowBox->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Fill)
		[
			SNew(SBox)
			.Clipping(EWidgetClipping::ClipToBounds)
			.WidthOverride_Lambda([Columns, bAligned]()
			{
				return bAligned ? Columns->GetNameWidth() : FOptionalSize();
			})
			[
				NameArea
			]
		];

		if (ValueWidget.IsValid())
		{
			RowBox->AddSlot()
			.AutoWidth()
			[
				MakeColumnLine()
			];

			// Value: fills the row (so the reset column lines up at the right edge), unless the group fixes its width
			const bool bFixedValueWidth = bAligned && Columns->GetValueWidth().IsSet();

			TSharedRef<SWidget> ValueCell = SNew(SBox)
				.WidthOverride_Lambda([Columns, bAligned]()
				{
					return bAligned ? Columns->GetValueWidth() : FOptionalSize();
				})
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				.Padding(FMargin(12, RowVerticalPadding, 4, RowVerticalPadding))
				[
					ValueWidget.ToSharedRef()
				];

			if (bFixedValueWidth)
			{
				RowBox->AddSlot()
				.AutoWidth()
				[
					ValueCell
				];
			}
			else
			{
				RowBox->AddSlot()
				.FillWidth(1.0f)
				[
					ValueCell
				];
			}
		}
		else
		{
			// Whole-row widgets span the name and value columns
			RowBox->AddSlot()
			.FillWidth(1.0f)
			[
				SNullWidget::NullWidget
			];
		}

		// Reset to default; always there (even if empty) so the column stays aligned
		RowBox->AddSlot()
		.AutoWidth()
		[
			MakeColumnLine()
		];

		const bool bHasResetButton = Handle.IsValid() && Handle->IsValidHandle() && Handle->GetProperty();

		RowBox->AddSlot()
		.AutoWidth()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("DetailsView.CategoryMiddle"))
			.BorderBackgroundColor_Lambda([IsRowHovered]()
			{
				return GetRowBackgroundColor(0, IsRowHovered.Get());
			})
			.Padding(0.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(ResetColumnWidth)
				.HAlign(HAlign_Center)
				[
					bHasResetButton ? MakeResetToDefaultButton(Handle.ToSharedRef(), Row) : SNullWidget::NullWidget
				]
			]
		];

		TSharedRef<SBorder> RowLine = SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("DetailsView.GridLine"))
			.Padding(FMargin(0, 0, 0, 1))
			[
				SNew(SBox)
				.MinDesiredHeight(RowMinHeight)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("DetailsView.CategoryMiddle"))
					.BorderBackgroundColor_Lambda([IsRowHovered, Depth]()
					{
						return GetRowBackgroundColor(Depth, IsRowHovered.Get());
					})
					.Padding(0.0f)
					[
						RowBox
					]
				]
			];

		*HoverSource = RowLine;

		// The row's own line, then its child rows (structs, arrays), which are rows of their own
		TSharedRef<SVerticalBox> RowWidget = SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				RowLine
			];

		if (ChildContainer.IsValid())
		{
			RowWidget->AddSlot()
			.AutoHeight()
			[
				ChildContainer.ToSharedRef()
			];
		}

		return SNew(SBox)
			.Visibility(MakeNodeVisibility(Node))
			[
				WrapIfNotInstanceEditable(RowWidget, Handle, *PropertyData)
			];
	}
}

#undef LOCTEXT_NAMESPACE
