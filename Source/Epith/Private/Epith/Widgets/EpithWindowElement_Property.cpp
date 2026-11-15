#include "Epith/Widgets/EpithWindowElement_Property.h"

#include "EpithPropertyRow.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"
#include "Epith/Widgets/EpithDrawContext.h"
#include "Styling/StyleColors.h"
#include "Widgets/SOverlay.h"

#define LOCTEXT_NAMESPACE "Epith"

namespace EpithPropertyElement
{
	TSharedRef<SWidget> MakeWarning(const FText& Message)
	{
		return SNew(STextBlock)
			.Text(Message)
			.ColorAndOpacity(FSlateColor(EStyleColor::Warning));
	}

	/** The row for PropertyPath, or a warning. Null if no path is set. */
	TSharedPtr<SWidget> MakeRow(FName PropertyPath, const FEpithDrawContext& Context)
	{
		if (PropertyPath.IsNone() || !Context.PropertyData.IsValid())
		{
			return nullptr;
		}

		TSharedPtr<IDetailTreeNode> TreeNode = Context.PropertyData->FindTreeNode(PropertyPath);

		if (!TreeNode.IsValid())
		{
			if (Context.IsDesigning())
			{
				// The designer previews against the class defaults, which don't have rows for instance-only properties.
				// That's expected there, so don't make it look like an error.
				return SNew(STextBlock)
					.Text(FText::Format(LOCTEXT("PropertyElement_NoPreview", "{0} (no preview)"), FText::FromName(PropertyPath)))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground());
			}

			return MakeWarning(FText::Format(LOCTEXT("PropertyElement_Missing", "Missing property: {0}"), FText::FromName(PropertyPath)));
		}

		// A property row's widgets can only live in one place. The details system builds them once per row and hands out the same
		// instances every time, and type customizations (e.g. the color picker) keep pointers to them. So each row may be drawn once
		// per container. (The original code called IDetailPropertyRow::GetDefaultWidgets as well, which re-runs CustomizeHeader and
		// repoints the customization at a second, hidden set of widgets; that is what made color pickers crash.)
		if (!Context.PropertyData->TryClaimPath(PropertyPath))
		{
			return MakeWarning(FText::Format(LOCTEXT("PropertyElement_Duplicate", "{0} is already shown in this panel"), FText::FromName(PropertyPath)));
		}

		return EpithPropertyRow::Make(TreeNode.ToSharedRef(), Context.PropertyData.ToSharedRef(), 0, Context.Columns);
	}
}

TSharedPtr<SWidget> FEpithWindowElement_Property::Draw(const FEpithDrawContext& Context) const
{
	const FName PropertyPath = PropertySelector.GetPropertyPath();

	TSharedPtr<SWidget> Row = EpithPropertyElement::MakeRow(PropertyPath, Context);

	if (!Context.IsDesigning())
	{
		return Row;
	}

	// Designer: a property picker until a property is chosen, then a preview of its row. (To choose another property,
	// right-click > Unset Property.) The preview is for looks only: it shows the class defaults and doesn't take clicks.
	FEpithPropertySelector* MutableSelector = const_cast<FEpithPropertySelector*>(&PropertySelector);

	TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox);

	if (Row.IsValid())
	{
		// In a group, drag handles over the column lines resize the group's columns
		TSharedRef<SOverlay> Preview = SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBox)
				.Visibility(EVisibility::HitTestInvisible)
				[
					Row.ToSharedRef()
				]
			];

		if (Context.Columns.IsValid())
		{
			Preview->AddSlot()
			[
				Context.Designer->MakeColumnHandles(Context.Columns.ToSharedRef())
			];
		}

		// Fills the width like rows in the real panel do, so the reset column sits at the right edge here too
		Box->AddSlot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		.Padding(0, 0, 8, 0)
		[
			Preview
		];
	}

	if (PropertyPath.IsNone())
	{
		Box->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			Context.Designer->MakePropertyPicker(PropertyPath, [MutableSelector](FName NewPath)
			{
				MutableSelector->SetPropertyPath(NewPath);
			})
		];
	}

	return Box;
}

#undef LOCTEXT_NAMESPACE
