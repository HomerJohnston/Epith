#include "EpithPropertyMenu.h"

#include "Editor.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"
#include "Epith/Widgets/EpithWindowElement_Property.h"
#include "Styling/AppStyle.h"
#include "Styling/StyleColors.h"
#include "StructUtils/InstancedStruct.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "UObject/UObjectHash.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "Epith"

namespace EpithPropertyMenu
{
	/**
	 * Finds an object to read TargetClass's property list from.
	 *
	 * We prefer a placed instance in the open level over the class default object, because the details system filters
	 * properties differently for the two: EditInstanceOnly properties are hidden on the CDO and EditDefaultsOnly ones are
	 * shown. Panels display placed instances, so an instance gives exactly the paths that will resolve at runtime.
	 */
	UObject* FindSampleObject(UClass* TargetClass, bool& bOutIsInstance)
	{
		bOutIsInstance = false;

		if (!TargetClass)
		{
			return nullptr;
		}

		UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;

		if (EditorWorld)
		{
			TArray<UObject*> Instances;
			GetObjectsOfClass(TargetClass, Instances, true, RF_ClassDefaultObject | RF_ArchetypeObject, EInternalObjectFlags::Garbage);

			UObject* SubclassInstance = nullptr;

			for (UObject* Instance : Instances)
			{
				if (!IsValid(Instance) || Instance->GetWorld() != EditorWorld)
				{
					continue;
				}

				// An instance of exactly TargetClass is ideal; a subclass instance works too (its extra properties are filtered out below)
				if (Instance->GetClass() == TargetClass)
				{
					bOutIsInstance = true;
					return Instance;
				}

				if (!SubclassInstance)
				{
					SubclassInstance = Instance;
				}
			}

			if (SubclassInstance)
			{
				bOutIsInstance = true;
				return SubclassInstance;
			}
		}

		return TargetClass->GetDefaultObject();
	}

	/** Walks up to the top-level property this handle lives under (e.g. MyStruct for MyStruct.Member). */
	const FProperty* GetTopLevelProperty(TSharedPtr<IPropertyHandle> Handle)
	{
		TSharedPtr<IPropertyHandle> Current = Handle;

		while (Current.IsValid())
		{
			TSharedPtr<IPropertyHandle> Parent = Current->GetParentHandle();

			if (!Parent.IsValid() || !Parent->GetProperty())
			{
				break;
			}

			Current = Parent;
		}

		return Current.IsValid() ? Current->GetProperty() : nullptr;
	}

	int32 GetInheritanceDepth(const UStruct* Struct)
	{
		int32 Depth = 0;

		for (const UStruct* Super = Struct; Super; Super = Super->GetSuperStruct())
		{
			++Depth;
		}

		return Depth;
	}

	/** Indent per nesting level: a struct's members sit one level in from the struct, and so on. */
	constexpr float IndentPerLevel = 12.0f;


	/** Tree lines down the left of a nested row: one indent column per level. */
	class SEpithTreeLines : public SLeafWidget
	{
	public:
		SLATE_BEGIN_ARGS(SEpithTreeLines) {}
			SLATE_ARGUMENT(TArray<ETreeLine>, Columns)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Columns = InArgs._Columns;
		}

		virtual FVector2D ComputeDesiredSize(float) const override
		{
			return FVector2D(Columns.Num() * IndentPerLevel, 0.0f);
		}

		virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
		{
			// Menu rows are padded inside their buttons; reach into that padding so the lines join up from row to row
			const FMargin BlockPadding = FAppStyle::Get().GetMargin("Menu", ".Block.Padding");
			const FMargin ButtonPadding = FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Menu.Button").NormalPadding;
			const float Above = BlockPadding.Top + ButtonPadding.Top;
			const float Below = BlockPadding.Bottom + ButtonPadding.Bottom;

			const float Height = AllottedGeometry.GetLocalSize().Y;
			const float MidY = FMath::RoundToFloat(Height * 0.5f);
			const FSlateBrush* Brush = FAppStyle::Get().GetBrush("WhiteBrush");
			const FLinearColor Color = FSlateColor::UseSubduedForeground().GetColor(InWidgetStyle) * FLinearColor(1.0f, 1.0f, 1.0f, 0.6f);

			auto DrawBox = [&](float X, float Y, float Width, float BoxHeight)
			{
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(FVector2f(Width, BoxHeight), FSlateLayoutTransform(FVector2f(X, Y))), Brush, ESlateDrawEffect::None, Color);
			};

			for (int32 i = 0; i < Columns.Num(); ++i)
			{
				const float ColumnLeft = i * IndentPerLevel;
				const float LineX = ColumnLeft + FMath::FloorToFloat(IndentPerLevel * 0.5f);
				const float BranchWidth = ColumnLeft + IndentPerLevel - LineX;

				switch (Columns[i])
				{
				case ETreeLine::Through:
					DrawBox(LineX, -Above, 1.0f, Above + Height + Below);
					break;

				case ETreeLine::Tee:
					DrawBox(LineX, -Above, 1.0f, Above + Height + Below);
					DrawBox(LineX, MidY, BranchWidth, 1.0f);
					break;

				case ETreeLine::Elbow:
					DrawBox(LineX, -Above, 1.0f, Above + MidY + 1.0f);
					DrawBox(LineX, MidY, BranchWidth, 1.0f);
					break;

				default:
					break;
				}
			}

			return LayerId;
		}

	private:
		TArray<ETreeLine> Columns;
	};

	/** "Struct.Vector.X" -> { "Struct", "Vector", "X" }. Array elements are their own level: "Numbers.Numbers[0]" -> { "Numbers", "Numbers[0]" }. */
	TArray<FString> GetPathSegments(FName Path)
	{
		TArray<FString> Segments;
		Path.ToString().ParseIntoArray(Segments, TEXT("."));
		return Segments;
	}

	/** Alphabetical, except array elements compare by index ("Numbers[2]" before "Numbers[10]"). */
	int32 CompareSegments(const FString& Left, const FString& Right)
	{
		int32 LeftBracket = INDEX_NONE;
		int32 RightBracket = INDEX_NONE;

		if (Left.FindChar(TEXT('['), LeftBracket) && Right.FindChar(TEXT('['), RightBracket))
		{
			const int32 NameOrder = Left.Left(LeftBracket).Compare(Right.Left(RightBracket), ESearchCase::IgnoreCase);

			if (NameOrder != 0)
			{
				return NameOrder;
			}

			return FCString::Atoi(*Left + LeftBracket + 1) - FCString::Atoi(*Right + RightBracket + 1);
		}

		return Left.Compare(Right, ESearchCase::IgnoreCase);
	}

	/** Tree order: a property comes right before the properties nested in it, and siblings are alphabetical. */
	bool IsBeforeInTree(const TArray<FString>& Left, const TArray<FString>& Right)
	{
		const int32 SharedNum = FMath::Min(Left.Num(), Right.Num());

		for (int32 i = 0; i < SharedNum; ++i)
		{
			const int32 Order = CompareSegments(Left[i], Right[i]);

			if (Order != 0)
			{
				return Order < 0;
			}
		}

		// One is nested in the other: the parent comes first
		return Left.Num() < Right.Num();
	}
}

TSharedRef<SWidget> EpithPropertyMenu::MakeMessage(const FText& Message)
{
	return SNew(SBox)
		.Padding(8)
		.MaxDesiredWidth(400)
		[
			SNew(STextBlock)
			.Text(Message)
			.AutoWrapText(true)
		];
}

namespace EpithPropertyMenuPrivate
{
	void GatherFromStruct(const UScriptStruct* Struct, const void* Data, TSet<FName>& OutPaths, int32 Depth);

	void GatherFromValue(const FProperty* Property, const void* Value, TSet<FName>& OutPaths, int32 Depth)
	{
		if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			// Child elements live in instanced structs (TInstancedStruct is laid out as an FInstancedStruct)
			if (StructProperty->Struct == FInstancedStruct::StaticStruct())
			{
				const FInstancedStruct* Instanced = static_cast<const FInstancedStruct*>(Value);
				GatherFromStruct(Instanced->GetScriptStruct(), Instanced->GetMemory(), OutPaths, Depth + 1);
			}
			else
			{
				GatherFromStruct(StructProperty->Struct, Value, OutPaths, Depth + 1);
			}
		}
		else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			FScriptArrayHelper Array(ArrayProperty, Value);

			for (int32 i = 0; i < Array.Num(); ++i)
			{
				GatherFromValue(ArrayProperty->Inner, Array.GetRawPtr(i), OutPaths, Depth + 1);
			}
		}
	}

	void GatherFromStruct(const UScriptStruct* Struct, const void* Data, TSet<FName>& OutPaths, int32 Depth)
	{
		if (!Struct || !Data || Depth > 64)
		{
			return;
		}

		if (Struct->IsChildOf(FEpithWindowElement_Property::StaticStruct()))
		{
			const FName Path = static_cast<const FEpithWindowElement_Property*>(Data)->PropertySelector.GetPropertyPath();

			if (!Path.IsNone())
			{
				OutPaths.Add(Path);
			}
		}

		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			GatherFromValue(*It, It->ContainerPtrToValuePtr<void>(Data), OutPaths, Depth);
		}
	}
}

TSet<FName> EpithPropertyMenu::GatherUsedPaths(const UEpithPanelDefinition* Definition)
{
	TSet<FName> Paths;

	if (const TInstancedStruct<FEpithWindowElement>* Root = Definition ? Definition->GetRoot() : nullptr)
	{
		EpithPropertyMenuPrivate::GatherFromStruct(Root->GetScriptStruct(), Root->GetMemory(), Paths, 0);
	}

	return Paths;
}

TSharedRef<SWidget> EpithPropertyMenu::MakeTreeLines(const TArray<ETreeLine>& Columns)
{
	return SNew(SEpithTreeLines).Columns(Columns);
}

FText EpithPropertyMenu::GetFromDefaultsNote(const UClass* TargetClass)
{
	return FText::Format(
		LOCTEXT("PropertySelector_UsingDefaults", "No {0} found in the open level, so this list comes from the class defaults. Instance-only properties are missing and some defaults-only properties may not appear in the panel. Place one in the level for an exact list."),
		TargetClass ? TargetClass->GetDisplayNameText() : FText::GetEmpty());
}

FText EpithPropertyMenu::AddUsedNote(const FText& ToolTip)
{
	return FText::Format(LOCTEXT("PropertySelector_AlreadyUsed", "{0}\n\nAlready shown by another Property element in this panel."), ToolTip);
}

FSlateColor EpithPropertyMenu::GetRowColor(bool bUsed, bool bInherited)
{
	// Green: already shown by a Property element in this panel. Dimmer: inherited from a parent class. Both: dimmer green.
	return bUsed ? FSlateColor(EStyleColor::AccentGreen) : bInherited ? FSlateColor::UseSubduedForeground() : FSlateColor::UseForeground();
}

float EpithPropertyMenu::GetRowOpacity(bool bUsed, bool bInherited)
{
	return bUsed && bInherited ? 0.6f : 1.0f;
}

EpithPropertyMenu::FList EpithPropertyMenu::GatherList(UClass* TargetClass)
{
	FList List;

	if (!TargetClass)
	{
		return List;
	}

	UObject* SampleObject = FindSampleObject(TargetClass, List.bFromInstance);

	FEpithPropertyDataContainer PropertyData(SampleObject);

	// One section per class that declares top-level properties (the target class and its parents). Properties nested in a
	// struct, array etc. are listed right under that property, indented, in the same section.
	struct FGatheredRow
	{
		FName Path;
		TArray<FString> Segments;
		TSharedPtr<IPropertyHandle> Handle;
		const UClass* InheritedFrom = nullptr;
	};

	TMap<const UClass*, TArray<FGatheredRow>> RowsBySection;

	for (const auto& [Path, Handle] : PropertyData.GetPropertyHandles())
	{
		const FProperty* Property = Handle.IsValid() ? Handle->GetProperty() : nullptr;

		if (!Property)
		{
			continue;
		}

		// Array, set and map elements (and anything inside them) can't be picked: only the whole container
		if (FEpithPropertyDataContainer::IsContainerElementPath(Path))
		{
			continue;
		}

		// If the sample is a subclass instance, skip properties that only exist on that subclass
		const FProperty* TopLevelProperty = GetTopLevelProperty(Handle);
		const UClass* TopLevelOwnerClass = TopLevelProperty ? TopLevelProperty->GetOwnerClass() : nullptr;

		if (TopLevelOwnerClass && !TargetClass->IsChildOf(TopLevelOwnerClass))
		{
			continue;
		}

		// Declared on a parent class of the target class, not on the target class itself
		const UClass* InheritedFrom = TopLevelOwnerClass && TopLevelOwnerClass != TargetClass ? TopLevelOwnerClass : nullptr;

		RowsBySection.FindOrAdd(TopLevelOwnerClass).Add({ Path, GetPathSegments(Path), Handle, InheritedFrom });
	}

	// The target class first, then its parent, that class's parent, and so on. Anything without a top-level class goes last.
	TArray<const UClass*> SectionClasses;
	RowsBySection.GetKeys(SectionClasses);

	// Taken out before sorting (TArray::Sort dereferences pointers) and put back at the end
	SectionClasses.Remove(nullptr);

	SectionClasses.Sort([] (const UClass& Left, const UClass& Right)
	{
		return GetInheritanceDepth(&Left) > GetInheritanceDepth(&Right);
	});

	if (RowsBySection.Contains(nullptr))
	{
		SectionClasses.Add(nullptr);
	}

	for (const UClass* SectionClass : SectionClasses)
	{
		TArray<FGatheredRow>& Rows = RowsBySection[SectionClass];

		Rows.Sort([] (const FGatheredRow& Left, const FGatheredRow& Right)
		{
			return IsBeforeInTree(Left.Segments, Right.Segments);
		});

		// Work out the tree lines. A row is the last of its siblings if the next row at its level or above isn't a sibling.
		auto GetDepth = [](const FGatheredRow& Row) { return FMath::Max(0, Row.Segments.Num() - 1); };

		TArray<bool> IsLastSibling;
		IsLastSibling.Init(true, Rows.Num());

		for (int32 i = 0; i < Rows.Num(); ++i)
		{
			const int32 Depth = GetDepth(Rows[i]);

			for (int32 k = i + 1; k < Rows.Num(); ++k)
			{
				const int32 OtherDepth = GetDepth(Rows[k]);

				if (OtherDepth < Depth)
				{
					break;
				}

				if (OtherDepth == Depth)
				{
					bool bSameParent = true;

					for (int32 Level = 0; Level < Depth && bSameParent; ++Level)
					{
						bSameParent = Rows[i].Segments[Level].Equals(Rows[k].Segments[Level], ESearchCase::CaseSensitive);
					}

					IsLastSibling[i] = !bSameParent;
					break;
				}
			}
		}

		// Whether the most recent row at each depth was the last of its siblings; its column keeps a line going if it wasn't
		TArray<bool> LastAtDepth;

		FListSection& Section = List.Sections.AddDefaulted_GetRef();
		Section.Heading = SectionClass ? SectionClass->GetDisplayNameText() : LOCTEXT("PropertySelector_OtherSection", "Other");

		for (int32 RowIndex = 0; RowIndex < Rows.Num(); ++RowIndex)
		{
			const FGatheredRow& Gathered = Rows[RowIndex];
			FListRow& Row = Section.Rows.AddDefaulted_GetRef();

			Row.Path = Gathered.Path;
			Row.InheritedFrom = Gathered.InheritedFrom;

			// Nested rows sit under the property they belong to, so they only need their own name
			Row.Label = FText::FromString(Gathered.Segments.Num() > 0 ? Gathered.Segments.Last() : Gathered.Path.ToString());
			Row.NestingLevel = GetDepth(Gathered);

			for (int32 Level = 1; Level < Row.NestingLevel; ++Level)
			{
				const bool bAncestorWasLast = !LastAtDepth.IsValidIndex(Level) || LastAtDepth[Level];
				Row.TreeColumns.Add(bAncestorWasLast ? ETreeLine::Empty : ETreeLine::Through);
			}

			if (Row.NestingLevel > 0)
			{
				Row.TreeColumns.Add(IsLastSibling[RowIndex] ? ETreeLine::Elbow : ETreeLine::Tee);
			}

			// Levels with no row drawn (e.g. a parent the details system hides) count as finished, so they get no line
			while (LastAtDepth.Num() <= Row.NestingLevel)
			{
				LastAtDepth.Add(true);
			}

			LastAtDepth[Row.NestingLevel] = IsLastSibling[RowIndex];

			Row.ToolTip = FText::Format(LOCTEXT("PropertySelector_PathToolTip", "{0}\n\nPath: {1}"), Gathered.Handle->GetToolTipText(), FText::FromName(Gathered.Path));

			if (FEpithPropertyDataContainer::FindInstanceEditBlocker(Gathered.Handle))
			{
				Row.ToolTip = FText::Format(LOCTEXT("PropertySelector_ReadOnlyNote", "{0}\n\nNot editable on placed instances, so it will be read-only in the panel."), Row.ToolTip);
			}

			if (Row.InheritedFrom)
			{
				Row.ToolTip = FText::Format(LOCTEXT("PropertySelector_Inherited", "{0}\n\nInherited from {1}."), Row.ToolTip, Row.InheritedFrom->GetDisplayNameText());
			}
		}
	}

	return List;
}

TSharedRef<SWidget> EpithPropertyMenu::Make(UClass* TargetClass, TFunction<void(FName)> OnPicked, const TSet<FName>& UsedPaths)
{
	if (!TargetClass)
	{
		return MakeMessage(LOCTEXT("PropertySelector_NoTargetClass", "Set the panel definition's Target Class first."));
	}

	const FList List = GatherList(TargetClass);

	FMenuBuilder MenuBuilder(true, nullptr);

	if (!List.bFromInstance)
	{
		MenuBuilder.AddWidget(MakeMessage(GetFromDefaultsNote(TargetClass)), FText::GetEmpty());
	}

	if (List.Sections.Num() == 0)
	{
		MenuBuilder.AddWidget(MakeMessage(LOCTEXT("PropertySelector_NoProperties", "No editable properties found.")), FText::GetEmpty());
	}

	for (const FListSection& Section : List.Sections)
	{
		MenuBuilder.BeginSection(NAME_None, Section.Heading);

		for (const FListRow& Row : Section.Rows)
		{
			const FName Path = Row.Path;
			const bool bUsed = UsedPaths.Contains(Path);
			const bool bInherited = Row.InheritedFrom != nullptr;

			const FUIAction PickAction(FExecuteAction::CreateLambda([OnPicked, Path]
			{
				OnPicked(Path);
			}));

			MenuBuilder.AddMenuEntry(
				PickAction,
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Fill)
				[
					MakeTreeLines(Row.TreeColumns)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(Row.NestingLevel > 0 ? 3.0f : 0.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(Row.Label)
					.TextStyle(&FAppStyle::Get(), "Menu.Label")
					.ColorAndOpacity(GetRowColor(bUsed, bInherited))
					.RenderOpacity(GetRowOpacity(bUsed, bInherited))
				],
				NAME_None,
				bUsed ? AddUsedNote(Row.ToolTip) : Row.ToolTip);
		}

		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
