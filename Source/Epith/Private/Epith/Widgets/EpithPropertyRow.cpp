#include "EpithPropertyRow.h"

#include "DetailTreeNode.h"
#include "DetailWidgetRow.h"
#include "IDetailTreeNode.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/StyleColors.h"
#include "UObject/UnrealType.h"
#include "Widgets/SBoxPanel.h"
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
	constexpr float IndentPerLevel = 12.0f;
	constexpr float ExpanderWidth = 16.0f;

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

	TSharedRef<SWidget> MakeChildRows(const TArray<TSharedRef<IDetailTreeNode>>& Children, const TSharedRef<FEpithPropertyDataContainer>& PropertyData, int32 Depth)
	{
		TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);

		for (const TSharedRef<IDetailTreeNode>& Child : Children)
		{
			const FName ChildKey = GetNodeKey(Child, Child->CreatePropertyHandle());

			TSharedRef<SWidget> ChildWidget = PropertyData->TryClaimChildPath(ChildKey)
				? Make(Child, PropertyData, Depth)
				: MakeWarning(FText::Format(LOCTEXT("PropertyRow_ChildDuplicate", "{0} is already shown in this panel"), FText::FromName(ChildKey)), Depth);

			Box->AddSlot()
			.AutoHeight()
			.Padding(0, 1)
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
			
			PinnedContainer->SetContent(MakeChildRows(Children, PinnedData.ToSharedRef(), Depth + 1));
			bBuilt = true;
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
	
	TSharedRef<SWidget> Make(TSharedRef<IDetailTreeNode> Node, TSharedRef<FEpithPropertyDataContainer> PropertyData, int32 Depth)
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
					WrapIfNotInstanceEditable(MakeChildRows(Children, PropertyData, Depth), Handle, *PropertyData)
				];
		}

		FDetailWidgetRow Row;
		const bool bHasRowContent = DetailNode.GenerateStandaloneWidget(Row) && Row.HasAnyContent();

		TSharedRef<SHorizontalBox> RowBox = SNew(SHorizontalBox);

		if (Depth > 0)
		{
			RowBox->AddSlot()
			.AutoWidth()
			[
				SNew(SSpacer)
				.Size(FVector2D(Depth * IndentPerLevel, 1))
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
			
			RowBox->AddSlot()
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
		else if (Depth > 0)
		{
			RowBox->AddSlot()
			.AutoWidth()
			[
				SNew(SSpacer)
				.Size(FVector2D(ExpanderWidth, 1))
			];
		}

		if (Row.OnEditConditionValueChanged.IsBound())
		{
			RowBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 4, 0)
			[
				MakeEditConditionToggle(Row)
			];
		}

		if (!bHasRowContent)
		{
			// Nothing to show for the row itself (rare); fall back to the property's name so its children still have a header
			RowBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Handle.IsValid() ? Handle->GetPropertyDisplayName() : FText::FromName(Key))
			];
		}
		else if (Row.HasColumns())
		{
			RowBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0, 0, 8, 0)
			[
				Row.NameWidget.Widget
			];

			RowBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				Row.ValueWidget.Widget
			];
		}
		else
		{
			RowBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				Row.WholeRowWidget.Widget
			];
		}

		if (Handle.IsValid() && Handle->IsValidHandle() && Handle->GetProperty())
		{
			RowBox->AddSlot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4, 0, 0, 0)
			[
				MakeResetToDefaultButton(Handle.ToSharedRef(), Row)
			];
		}

		TSharedRef<SWidget> RowWidget = RowBox;

		if (ChildContainer.IsValid())
		{
			RowWidget = SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					RowBox
				]
				+ SVerticalBox::Slot()
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
