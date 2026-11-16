#include "EpithPropertyMenu.h"

#include "Editor.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"
#include "Epith/Widgets/EpithWindowElement_Property.h"
#include "Styling/StyleColors.h"
#include "StructUtils/InstancedStruct.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "UObject/UObjectHash.h"
#include "Widgets/Layout/SBox.h"
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

	/** Splits "Struct.Vector.X" into the path of the property it's nested in ("Struct.Vector") and its own name ("X"). */
	void SplitPath(const FString& Path, FString& OutParentPath, FString& OutName)
	{
		int32 LastDot = INDEX_NONE;

		if (Path.FindLastChar(TEXT('.'), LastDot))
		{
			OutParentPath = Path.Left(LastDot);
			OutName = Path.Mid(LastDot + 1);
		}
		else
		{
			OutParentPath.Reset();
			OutName = Path;
		}
	}

	/**
	 * "Some Class", or for nested properties "Some Class ➝ Struct ➝ Vector": the class, then each property the entries are nested in.
	 * Array elements show as "Numbers[0]" (the property editor's path repeats the array's name for its elements: "Numbers.Numbers[0]").
	 */
	FText MakeSectionHeading(const UClass* TopLevelClass, const FString& ParentPath)
	{
		TArray<FString> Segments;
		ParentPath.ParseIntoArray(Segments, TEXT("."));

		TArray<FString> Names;

		for (const FString& Segment : Segments)
		{
			int32 BracketIndex = INDEX_NONE;
			const FString PropertyName = Segment.FindChar(TEXT('['), BracketIndex) ? Segment.Left(BracketIndex) : Segment;
			const FString IndexSuffix = BracketIndex != INDEX_NONE ? Segment.Mid(BracketIndex) : FString();
			const FString DisplayName = FName::NameToDisplayString(PropertyName, false) + IndexSuffix;

			// An element of the previous segment's array replaces it: "Numbers ➝ Numbers[0]" becomes "Numbers[0]"
			if (!IndexSuffix.IsEmpty() && Names.Num() > 0 && Names.Last() == FName::NameToDisplayString(PropertyName, false))
			{
				Names.Last() = DisplayName;
			}
			else
			{
				Names.Add(DisplayName);
			}
		}

		FString Heading = TopLevelClass ? TopLevelClass->GetDisplayNameText().ToString() : LOCTEXT("PropertySelector_OtherSection", "Other").ToString();

		for (const FString& Name : Names)
		{
			Heading += TEXT(" \u279D ") + Name;
		}

		return FText::FromString(Heading);
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

TSharedRef<SWidget> EpithPropertyMenu::Make(UClass* TargetClass, TFunction<void(FName)> OnPicked, const TSet<FName>& UsedPaths)
{
	if (!TargetClass)
	{
		return MakeMessage(LOCTEXT("PropertySelector_NoTargetClass", "Set the panel definition's Target Class first."));
	}

	bool bIsInstance = false;
	UObject* SampleObject = FindSampleObject(TargetClass, bIsInstance);

	FEpithPropertyDataContainer PropertyData(SampleObject);

	// Group property paths into sections: first by the class that declares their top-level property (the target class or one of
	// its parents), then by the property they're nested in. So Position's X, Y and Z get a "Some Class ➝ Position" section, under
	// the target class and ahead of anything inherited.
	using FPropertyHandlesByPath = TMap<FName, TSharedPtr<IPropertyHandle>>;
	using FSectionKey = TPair<const UClass*, FString>; // (top-level owner class, path of the property the entries are nested in)
	TMap<FSectionKey, FPropertyHandlesByPath> HandlesBySection;

	// Paths whose top-level property is declared on a parent class of the target class, not on the target class itself
	TMap<FName, const UClass*> InheritedFrom;

	for (const auto& [Path, Handle] : PropertyData.GetPropertyHandles())
	{
		const FProperty* Property = Handle.IsValid() ? Handle->GetProperty() : nullptr;

		if (!Property)
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

		if (TopLevelOwnerClass && TopLevelOwnerClass != TargetClass)
		{
			InheritedFrom.Add(Path, TopLevelOwnerClass);
		}

		FString ParentPath;
		FString Name;
		SplitPath(Path.ToString(), ParentPath, Name);

		HandlesBySection.FindOrAdd(FSectionKey(TopLevelOwnerClass, ParentPath)).Add(Path, Handle);
	}

	// The target class's own properties, then the sections for its properties' members (structs etc.), then the same for its
	// parent class, that class's parent, and so on.
	TArray<FSectionKey> Sections;
	HandlesBySection.GetKeys(Sections);

	Sections.Sort([] (const FSectionKey& Left, const FSectionKey& Right)
	{
		// Most-derived class first; anything without a top-level class goes last
		const int32 LeftDepth = Left.Key ? GetInheritanceDepth(Left.Key) : -1;
		const int32 RightDepth = Right.Key ? GetInheritanceDepth(Right.Key) : -1;

		if (LeftDepth != RightDepth)
		{
			return LeftDepth > RightDepth;
		}

		// Within one class: the class's own properties (no parent path) first, then nested sections by path, so each one
		// follows the section of the property it's nested in ("Struct" < "Struct.Vector")
		const bool bLeftIsOwn = Left.Value.IsEmpty();
		const bool bRightIsOwn = Right.Value.IsEmpty();

		if (bLeftIsOwn != bRightIsOwn)
		{
			return bLeftIsOwn;
		}

		return Left.Value.Compare(Right.Value, ESearchCase::CaseSensitive) < 0;
	});

	FMenuBuilder MenuBuilder(true, nullptr);

	if (!bIsInstance)
	{
		MenuBuilder.AddWidget(MakeMessage(FText::Format(
			LOCTEXT("PropertySelector_UsingDefaults", "No {0} found in the open level, so this list comes from the class defaults. Instance-only properties are missing and some defaults-only properties may not appear in the panel. Place one in the level for an exact list."),
			TargetClass->GetDisplayNameText())), FText::GetEmpty());
	}

	if (Sections.Num() == 0)
	{
		MenuBuilder.AddWidget(MakeMessage(LOCTEXT("PropertySelector_NoProperties", "No editable properties found.")), FText::GetEmpty());
	}

	for (const FSectionKey& Section : Sections)
	{
		FPropertyHandlesByPath& Handles = HandlesBySection[Section];

		Handles.KeySort([] (const FName& Left, const FName& Right)
		{
			return Left.LexicalLess(Right);
		});

		MenuBuilder.BeginSection(NAME_None, MakeSectionHeading(Section.Key, Section.Value));

		for (const auto& [Path, Handle] : Handles)
		{
			// The heading names the class and the properties above this one, so the entry only needs its own name
			FString ParentPath;
			FString Name;
			SplitPath(Path.ToString(), ParentPath, Name);
			const FText Label = FText::FromString(Name);

			FText ToolTip = FText::Format(LOCTEXT("PropertySelector_PathToolTip", "{0}\n\nPath: {1}"), Handle->GetToolTipText(), FText::FromName(Path));

			if (FEpithPropertyDataContainer::FindInstanceEditBlocker(Handle))
			{
				ToolTip = FText::Format(LOCTEXT("PropertySelector_ReadOnlyNote", "{0}\n\nNot editable on placed instances, so it will be read-only in the panel."), ToolTip);
			}

			const FUIAction PickAction(FExecuteAction::CreateLambda([OnPicked, Path]
			{
				OnPicked(Path);
			}));

			const bool bUsed = UsedPaths.Contains(Path);
			const UClass* const* ParentClass = InheritedFrom.Find(Path);

			if (bUsed)
			{
				ToolTip = FText::Format(LOCTEXT("PropertySelector_AlreadyUsed", "{0}\n\nAlready shown by another Property element in this panel."), ToolTip);
			}

			if (ParentClass)
			{
				ToolTip = FText::Format(LOCTEXT("PropertySelector_Inherited", "{0}\n\nInherited from {1}."), ToolTip, (*ParentClass)->GetDisplayNameText());
			}

			if (bUsed || ParentClass)
			{
				// Green: already shown by another Property element in this panel. Dimmer: inherited from a parent class.
				// Both: dimmer green.
				MenuBuilder.AddMenuEntry(
					PickAction,
					SNew(STextBlock)
					.Text(Label)
					.ColorAndOpacity(bUsed ? FSlateColor(EStyleColor::AccentGreen) : FSlateColor::UseSubduedForeground())
					.RenderOpacity(bUsed && ParentClass ? 0.6f : 1.0f),
					NAME_None,
					ToolTip);
			}
			else
			{
				MenuBuilder.AddMenuEntry(
					Label,
					ToolTip,
					FSlateIcon(),
					PickAction);
			}
		}

		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
