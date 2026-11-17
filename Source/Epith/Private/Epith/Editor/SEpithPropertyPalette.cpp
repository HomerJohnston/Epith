#include "SEpithPropertyPalette.h"

#include "EpithPropertyMenu.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "Epith"

TSharedRef<FEpithPropertyDragDrop> FEpithPropertyDragDrop::New(FName InPropertyPath, const FText& InLabel)
{
	TSharedRef<FEpithPropertyDragDrop> Operation = MakeShared<FEpithPropertyDragDrop>();
	Operation->PropertyPath = InPropertyPath;
	Operation->Label = InLabel;
	Operation->CurrentHoverText = InLabel;
	Operation->CurrentIconBrush = FAppStyle::GetBrush("Graph.ConnectorFeedback.Error");
	Operation->SetupDefaults();
	Operation->Construct();
	return Operation;
}

TSharedRef<FEpithElementTypeDragDrop> FEpithElementTypeDragDrop::New(const UScriptStruct* InElementType)
{
	TSharedRef<FEpithElementTypeDragDrop> Operation = MakeShared<FEpithElementTypeDragDrop>();
	Operation->ElementType = InElementType;
	Operation->Label = InElementType ? InElementType->GetDisplayNameText() : FText::GetEmpty();
	Operation->CurrentHoverText = Operation->Label;
	Operation->CurrentIconBrush = FAppStyle::GetBrush("Graph.ConnectorFeedback.Error");
	Operation->SetupDefaults();
	Operation->Construct();
	return Operation;
}

void SEpithPaletteRow::Construct(const FArguments& InArgs)
{
	OnBeginDrag = InArgs._OnBeginDrag;

	const int32 NestingLevel = InArgs._TreeColumns.Num();

	SetCursor(EMouseCursor::GrabHand);
	SetToolTipText(InArgs._RowToolTip);

	TSharedRef<SHorizontalBox> Content = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Fill)
		[
			EpithPropertyMenu::MakeTreeLines(InArgs._TreeColumns)
		];

	if (InArgs._Icon)
	{
		Content->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, 6.0f, 0.0f)
		[
			SNew(SImage)
			.Image(InArgs._Icon)
			.DesiredSizeOverride(FVector2D(16.0f, 16.0f))
			.ColorAndOpacity(FSlateColor::UseForeground())
		];
	}

	Content->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(NestingLevel > 0 ? 3.0f : 0.0f, 0.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(InArgs._Label)
			.ColorAndOpacity(InArgs._TextColor.IsSet() ? InArgs._TextColor : TAttribute<FSlateColor>(FSlateColor::UseForeground()))
			.RenderOpacity(InArgs._TextOpacity)
		];

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor_Lambda([this]()
		{
			return IsHovered() ? FSlateColor(FStyleColors::Hover) : FSlateColor(FLinearColor::Transparent);
		})
		// 3 px above and below: the tree lines reach that far, so they join up from row to row
		.Padding(FMargin(8.0f, 3.0f))
		[
			Content
		]
	];
}

FReply SEpithPaletteRow::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && OnBeginDrag)
	{
		return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
	}

	return FReply::Unhandled();
}

FReply SEpithPaletteRow::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return OnBeginDrag ? FReply::Handled().BeginDragDrop(OnBeginDrag()) : FReply::Unhandled();
}

namespace EpithPropertyPalette
{
	/** A class's heading, drawn like a menu section heading: capitals, with a line to the right. */
	TSharedRef<SWidget> MakeHeading(const FText& Heading)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Heading.ToUpper())
				.TextStyle(&FAppStyle::Get(), "Menu.Heading")
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SSeparator)
				.Orientation(Orient_Horizontal)
				.Thickness(1.0f)
			];
	}

	TSharedRef<SWidget> MakeMessage(const FText& Message)
	{
		return SNew(SBox)
			.Padding(FMargin(8.0f, 6.0f))
			[
				SNew(STextBlock)
				.Text(Message)
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];
	}
}

void SEpithPropertyPalette::Construct(const FArguments& InArgs)
{
	// No title or scroll box of its own: it's the body of the left pane's PROPERTIES section, and the pane scrolls
	ChildSlot
	[
		SAssignNew(ListBox, SVerticalBox)
	];
}

void SEpithPropertyPalette::Refresh(UClass* TargetClass, bool bForce)
{
	if (!bForce && bHasListed && ListedClass.Get() == TargetClass)
	{
		return;
	}

	ListedClass = TargetClass;
	bHasListed = true;
	Rebuild();
}

void SEpithPropertyPalette::SetUsedPaths(const TSet<FName>& InUsedPaths)
{
	*UsedPaths = InUsedPaths;
}

void SEpithPropertyPalette::Rebuild()
{
	if (!ListBox.IsValid())
	{
		return;
	}

	ListBox->ClearChildren();

	UClass* TargetClass = ListedClass.Get();

	if (!TargetClass)
	{
		ListBox->AddSlot()
		.AutoHeight()
		[
			EpithPropertyPalette::MakeMessage(LOCTEXT("PropertyPalette_NoTargetClass", "Pick a Target Class to list its properties."))
		];
		return;
	}

	const EpithPropertyMenu::FList List = EpithPropertyMenu::GatherList(TargetClass);

	if (!List.bFromInstance)
	{
		ListBox->AddSlot()
		.AutoHeight()
		[
			EpithPropertyPalette::MakeMessage(EpithPropertyMenu::GetFromDefaultsNote(TargetClass))
		];
	}

	if (List.Sections.Num() == 0)
	{
		ListBox->AddSlot()
		.AutoHeight()
		[
			EpithPropertyPalette::MakeMessage(LOCTEXT("PropertyPalette_NoProperties", "No editable properties found."))
		];
	}

	for (const EpithPropertyMenu::FListSection& Section : List.Sections)
	{
		ListBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(8.0f, 8.0f, 8.0f, 2.0f))
		[
			EpithPropertyPalette::MakeHeading(Section.Heading)
		];

		for (const EpithPropertyMenu::FListRow& Row : Section.Rows)
		{
			const bool bInherited = Row.InheritedFrom != nullptr;
			const FName Path = Row.Path;
			const FText Label = Row.Label;
			const FText ToolTip = Row.ToolTip;
			TSharedRef<TSet<FName>> Used = UsedPaths;

			ListBox->AddSlot()
			.AutoHeight()
			[
				SNew(SEpithPaletteRow)
				.Label(Label)
				.TreeColumns(Row.TreeColumns)
				// Green once a Property element in the panel shows it; dimmer if inherited
				.TextColor_Lambda([Used, Path, bInherited]()
				{
					return EpithPropertyMenu::GetRowColor(Used->Contains(Path), bInherited);
				})
				.TextOpacity(EpithPropertyMenu::GetRowOpacity(false, bInherited))
				.RowToolTip_Lambda([Used, Path, ToolTip]()
				{
					return Used->Contains(Path) ? EpithPropertyMenu::AddUsedNote(ToolTip) : ToolTip;
				})
				.OnBeginDrag([Path, Label]() -> TSharedRef<FDragDropOperation>
				{
					return FEpithPropertyDragDrop::New(Path, Label);
				})
			];
		}
	}
}

#undef LOCTEXT_NAMESPACE
