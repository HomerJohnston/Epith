#include "EpithPropertyMenu.h"

#include "Editor.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"
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

TSharedRef<SWidget> EpithPropertyMenu::Make(UClass* TargetClass, TFunction<void(FName)> OnPicked)
{
	if (!TargetClass)
	{
		return MakeMessage(LOCTEXT("PropertySelector_NoTargetClass", "Set the panel definition's Target Class first."));
	}
	
	bool bIsInstance = false;
	UObject* SampleObject = FindSampleObject(TargetClass, bIsInstance);
	
	FEpithPropertyDataContainer PropertyData(SampleObject);
	
	// Group property paths by the class or struct that declares them
	using FPropertyHandlesByPath = TMap<FName, TSharedPtr<IPropertyHandle>>;
	TMap<const UStruct*, FPropertyHandlesByPath> HandlesByOwner;
	
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
		
		const UStruct* OwnerStruct = Property->GetOwnerStruct();
		
		if (!OwnerStruct)
		{
			continue;
		}
		
		HandlesByOwner.FindOrAdd(OwnerStruct).Add(Path, Handle);
	}
	
	// Classes first, most-derived first; then structs alphabetically. (The old IsChildOf-based sort wasn't a valid ordering for unrelated types.)
	TArray<const UStruct*> Owners;
	HandlesByOwner.GetKeys(Owners);
	
	Owners.Sort([] (const UStruct& Left, const UStruct& Right)
	{
		const bool bLeftIsClass = Left.IsA<UClass>();
		const bool bRightIsClass = Right.IsA<UClass>();
		
		if (bLeftIsClass != bRightIsClass)
		{
			return bLeftIsClass;
		}
		
		if (bLeftIsClass)
		{
			const int32 LeftDepth = GetInheritanceDepth(&Left);
			const int32 RightDepth = GetInheritanceDepth(&Right);
			
			if (LeftDepth != RightDepth)
			{
				return LeftDepth > RightDepth;
			}
		}
		
		return Left.GetName() < Right.GetName();
	});
	
	FMenuBuilder MenuBuilder(true, nullptr);
	
	if (!bIsInstance)
	{
		MenuBuilder.AddWidget(MakeMessage(FText::Format(
			LOCTEXT("PropertySelector_UsingDefaults", "No {0} found in the open level, so this list comes from the class defaults. Instance-only properties are missing and some defaults-only properties may not appear in the panel. Place one in the level for an exact list."),
			TargetClass->GetDisplayNameText())), FText::GetEmpty());
	}
	
	if (Owners.Num() == 0)
	{
		MenuBuilder.AddWidget(MakeMessage(LOCTEXT("PropertySelector_NoProperties", "No editable properties found.")), FText::GetEmpty());
	}
	
	for (const UStruct* Owner : Owners)
	{
		FPropertyHandlesByPath& Handles = HandlesByOwner[Owner];
		
		Handles.KeySort([] (const FName& Left, const FName& Right)
		{
			return Left.LexicalLess(Right);
		});
		
		MenuBuilder.BeginSection(NAME_None, Owner->GetDisplayNameText());
		
		for (const auto& [Path, Handle] : Handles)
		{
			FText ToolTip = Handle->GetToolTipText();
			
			if (FEpithPropertyDataContainer::FindInstanceEditBlocker(Handle))
			{
				ToolTip = FText::Format(LOCTEXT("PropertySelector_ReadOnlyNote", "{0}\n\nNot editable on placed instances, so it will be read-only in the panel."), ToolTip);
			}
			
			MenuBuilder.AddMenuEntry(
				FText::FromName(Path),
				ToolTip,
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([OnPicked, Path]
				{
					OnPicked(Path);
				})));
		}
		
		MenuBuilder.EndSection();
	}
	
	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
