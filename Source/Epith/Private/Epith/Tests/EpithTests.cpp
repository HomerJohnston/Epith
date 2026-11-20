#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "EpithTestObject.h"
#include "IDetailTreeNode.h"
#include "PropertyHandle.h"
#include "Components/StaticMeshComponent.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/EditorWorldExtensions/EpithEditorWorldExtension.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"
#include "Epith/Widgets/EpithDrawContext.h"
#include "Epith/Widgets/EpithPropertyRow.h"
#include "Epith/Widgets/EpithWindowElement_Property.h"
#include "Epith/Widgets/EpithWindowElement_VerticalGroup.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Text/STextBlock.h"

// Run with BuildEpith.bat (test mode), or in the editor: Tools > Session Frontend > Automation, filter "Epith".

/** Reaches into Epith types the way the editor UI would, without adding test-only setters to them. */
struct FEpithTestAccess
{
	static UEpithPanelDefinition* MakeDefinition(UClass* TargetClass, bool bWithRoot = true, bool bSupplementary = false, bool bDisabled = false)
	{
		UEpithPanelDefinition* Definition = NewObject<UEpithPanelDefinition>(GetTransientPackage());
		Definition->TargetClass = TargetClass;
		Definition->bSupplementary = bSupplementary;
		Definition->bDisabled = bDisabled;

		if (bWithRoot)
		{
			Definition->Root.InitializeAs<FEpithWindowElement_VerticalGroup>();
		}

		return Definition;
	}

	static FEpithWindowElement_Property MakePropertyElement(FName PropertyPath)
	{
		FEpithWindowElement_Property Element;
		Element.PropertySelector.PropertyPath = PropertyPath;
		return Element;
	}
};

namespace EpithTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	/** Waits a number of editor frames, so row generators get ticked. */
	void WaitFrames(int32 NumFrames)
	{
		TSharedRef<int32> FramesLeft = MakeShared<int32>(NumFrames);

		ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([FramesLeft]()
		{
			return --(*FramesLeft) <= 0;
		}));
	}

	FString GetText(const TSharedPtr<SWidget>& Widget)
	{
		if (Widget.IsValid() && Widget->GetType() == FName(TEXT("STextBlock")))
		{
			return StaticCastSharedPtr<STextBlock>(Widget)->GetText().ToString();
		}

		return FString();
	}
}

// ================================================================================================
// Matching objects to panel definitions
// ================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEpithMatchingMostDerivedTest, "Epith.Matching.MostDerivedWins", EpithTests::Flags)

bool FEpithMatchingMostDerivedTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UEpithPanelDefinition> SceneDefinition(FEpithTestAccess::MakeDefinition(USceneComponent::StaticClass()));
	TStrongObjectPtr<UEpithPanelDefinition> PrimitiveDefinition(FEpithTestAccess::MakeDefinition(UPrimitiveComponent::StaticClass()));
	TStrongObjectPtr<UEpithPanelDefinition> NoRootDefinition(FEpithTestAccess::MakeDefinition(UStaticMeshComponent::StaticClass(), false));
	TStrongObjectPtr<UEpithPanelDefinition> NoTargetDefinition(FEpithTestAccess::MakeDefinition(nullptr));

	TStrongObjectPtr<UStaticMeshComponent> MeshComponent(NewObject<UStaticMeshComponent>(GetTransientPackage()));
	TStrongObjectPtr<USceneComponent> SceneComponent(NewObject<USceneComponent>(GetTransientPackage()));
	TStrongObjectPtr<UEpithTestObject> Unrelated(NewObject<UEpithTestObject>(GetTransientPackage()));

	// Order the definitions so the least derived comes first; the result must not depend on order
	const TArray<UEpithPanelDefinition*> Definitions = { SceneDefinition.Get(), NoTargetDefinition.Get(), PrimitiveDefinition.Get(), NoRootDefinition.Get() };
	const TArray<UObject*> Objects = { MeshComponent.Get(), SceneComponent.Get(), Unrelated.Get() };

	const TArray<FEpithObjectPanelMatch> Matches = UEpithEditorWorldExtension::FindPanelMatches(Objects, Definitions);

	TestEqual(TEXT("One match per object that has a definition, none for the unrelated object"), Matches.Num(), 2);

	for (const FEpithObjectPanelMatch& Match : Matches)
	{
		if (Match.Object == MeshComponent.Get())
		{
			TestTrue(TEXT("A static mesh component gets the primitive-component definition (most derived with a layout)"), Match.PanelDefinition == PrimitiveDefinition.Get());
		}
		else if (Match.Object == SceneComponent.Get())
		{
			TestTrue(TEXT("A scene component gets the scene-component definition"), Match.PanelDefinition == SceneDefinition.Get());
		}
		else
		{
			AddError(FString::Printf(TEXT("Unexpected match for %s"), *GetNameSafe(Match.Object)));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEpithMatchingDuplicateTest, "Epith.Matching.DuplicateTargetClassWarns", EpithTests::Flags)

bool FEpithMatchingDuplicateTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UEpithPanelDefinition> First(FEpithTestAccess::MakeDefinition(USceneComponent::StaticClass()));
	TStrongObjectPtr<UEpithPanelDefinition> Second(FEpithTestAccess::MakeDefinition(USceneComponent::StaticClass()));
	TStrongObjectPtr<USceneComponent> SceneComponent(NewObject<USceneComponent>(GetTransientPackage()));

	AddExpectedMessage(TEXT("both target class"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);

	const TArray<FEpithObjectPanelMatch> Matches = UEpithEditorWorldExtension::FindPanelMatches({ SceneComponent.Get() }, { First.Get(), Second.Get() });

	if (TestEqual(TEXT("Two definitions for the same class still give one pane"), Matches.Num(), 1))
	{
		TestTrue(TEXT("The first definition wins"), Matches[0].PanelDefinition == First.Get());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEpithMatchingSupplementaryTest, "Epith.Matching.SupplementaryAlwaysApplies", EpithTests::Flags)

bool FEpithMatchingSupplementaryTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UEpithPanelDefinition> SceneDefinition(FEpithTestAccess::MakeDefinition(USceneComponent::StaticClass()));
	TStrongObjectPtr<UEpithPanelDefinition> PrimitiveDefinition(FEpithTestAccess::MakeDefinition(UPrimitiveComponent::StaticClass()));
	TStrongObjectPtr<UEpithPanelDefinition> AnyComponent(FEpithTestAccess::MakeDefinition(UActorComponent::StaticClass(), true, true));
	TStrongObjectPtr<UEpithPanelDefinition> AnyScene(FEpithTestAccess::MakeDefinition(USceneComponent::StaticClass(), true, true));

	TStrongObjectPtr<UStaticMeshComponent> MeshComponent(NewObject<UStaticMeshComponent>(GetTransientPackage()));
	TStrongObjectPtr<UEpithTestObject> Unrelated(NewObject<UEpithTestObject>(GetTransientPackage()));

	// Supplementary definitions first and least derived first: neither order may matter
	const TArray<FEpithObjectPanelMatch> Matches = UEpithEditorWorldExtension::FindPanelMatches(
		{ MeshComponent.Get(), Unrelated.Get() },
		{ AnyComponent.Get(), AnyScene.Get(), SceneDefinition.Get(), PrimitiveDefinition.Get() });

	if (TestEqual(TEXT("The main panel plus both supplementary panels, nothing for the unrelated object"), Matches.Num(), 3))
	{
		// Most general target class first: ActorComponent, SceneComponent, then the PrimitiveComponent main panel
		TestTrue(TEXT("The most general supplementary panel comes first"), Matches[0].PanelDefinition == AnyComponent.Get());
		TestTrue(TEXT("Then the next most general"), Matches[1].PanelDefinition == AnyScene.Get());
		TestTrue(TEXT("The most specific (main) panel comes last"), Matches[2].PanelDefinition == PrimitiveDefinition.Get());
	}

	// A supplementary panel on its own is enough to show a panel
	const TArray<FEpithObjectPanelMatch> OnlySupplementary = UEpithEditorWorldExtension::FindPanelMatches({ MeshComponent.Get() }, { AnyComponent.Get() });
	TestEqual(TEXT("A supplementary panel applies even with no main panel"), OnlySupplementary.Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEpithMatchingDisabledTest, "Epith.Matching.DisabledIsIgnored", EpithTests::Flags)

bool FEpithMatchingDisabledTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UEpithPanelDefinition> SceneDefinition(FEpithTestAccess::MakeDefinition(USceneComponent::StaticClass()));
	TStrongObjectPtr<UEpithPanelDefinition> DisabledPrimitive(FEpithTestAccess::MakeDefinition(UPrimitiveComponent::StaticClass(), true, false, true));
	TStrongObjectPtr<UEpithPanelDefinition> DisabledSupplementary(FEpithTestAccess::MakeDefinition(UActorComponent::StaticClass(), true, true, true));

	TStrongObjectPtr<UStaticMeshComponent> MeshComponent(NewObject<UStaticMeshComponent>(GetTransientPackage()));

	const TArray<FEpithObjectPanelMatch> Matches = UEpithEditorWorldExtension::FindPanelMatches(
		{ MeshComponent.Get() },
		{ SceneDefinition.Get(), DisabledPrimitive.Get(), DisabledSupplementary.Get() });

	// The disabled, more specific definition doesn't win over the enabled one, and the disabled supplementary one adds nothing
	if (TestEqual(TEXT("Only the enabled definition matches"), Matches.Num(), 1))
	{
		TestTrue(TEXT("The enabled scene-component definition is used"), Matches[0].PanelDefinition == SceneDefinition.Get());
	}

	// Two definitions for the same class, one disabled: no duplicate warning, the enabled one is used
	TStrongObjectPtr<UEpithPanelDefinition> DisabledScene(FEpithTestAccess::MakeDefinition(USceneComponent::StaticClass(), true, false, true));
	const TArray<FEpithObjectPanelMatch> NoDuplicate = UEpithEditorWorldExtension::FindPanelMatches({ MeshComponent.Get() }, { DisabledScene.Get(), SceneDefinition.Get() });

	if (TestEqual(TEXT("A disabled duplicate is ignored"), NoDuplicate.Num(), 1))
	{
		TestTrue(TEXT("The enabled duplicate is used"), NoDuplicate[0].PanelDefinition == SceneDefinition.Get());
	}

	return true;
}

// ================================================================================================
// Property data container
// ================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEpithPropertyDataFindsRowsTest, "Epith.PropertyData.FindsRows", EpithTests::Flags)

bool FEpithPropertyDataFindsRowsTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UEpithTestObject> Object(NewObject<UEpithTestObject>(GetTransientPackage()));
	Object->bCondition = false;

	FEpithPropertyDataContainer PropertyData(Object.Get());

	TestTrue(TEXT("Plain property"), PropertyData.FindTreeNode(TEXT("PlainFloat")).IsValid());
	TestTrue(TEXT("EditCondition property"), PropertyData.FindTreeNode(TEXT("GreyedWhenFalse")).IsValid());
	TestTrue(TEXT("EditConditionHides property is findable while hidden"), PropertyData.FindTreeNode(TEXT("HiddenWhenFalse")).IsValid());
	TestTrue(TEXT("Inline-edit-condition property"), PropertyData.FindTreeNode(TEXT("InlineGated")).IsValid());
	TestTrue(TEXT("EditDefaultsOnly property on an instance"), PropertyData.FindTreeNode(TEXT("DefaultsOnly")).IsValid());
	TestTrue(TEXT("Struct"), PropertyData.FindTreeNode(TEXT("Struct")).IsValid());
	TestTrue(TEXT("Struct member"), PropertyData.FindTreeNode(TEXT("Struct.Value")).IsValid());
	TestTrue(TEXT("Array"), PropertyData.FindTreeNode(TEXT("Numbers")).IsValid());
	TestTrue(TEXT("Color (customized struct)"), PropertyData.FindTreeNode(TEXT("Color")).IsValid());
	TestFalse(TEXT("A path that doesn't exist"), PropertyData.FindTreeNode(TEXT("DoesNotExist")).IsValid());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEpithPropertyDataInstanceEditTest, "Epith.PropertyData.InstanceEditBlocker", EpithTests::Flags)

bool FEpithPropertyDataInstanceEditTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UEpithTestObject> Object(NewObject<UEpithTestObject>(GetTransientPackage()));

	FEpithPropertyDataContainer InstanceData(Object.Get());
	FEpithPropertyDataContainer DefaultsData(UEpithTestObject::StaticClass()->GetDefaultObject());

	TestTrue(TEXT("A NewObject is an instance"), InstanceData.IsEditingInstance());
	TestFalse(TEXT("The CDO is not an instance"), DefaultsData.IsEditingInstance());

	TestNull(TEXT("EditAnywhere isn't blocked"), FEpithPropertyDataContainer::FindInstanceEditBlocker(InstanceData.FindPropertyHandle(TEXT("PlainFloat"))));

	const FProperty* DefaultsOnlyBlocker = FEpithPropertyDataContainer::FindInstanceEditBlocker(InstanceData.FindPropertyHandle(TEXT("DefaultsOnly")));
	TestTrue(TEXT("EditDefaultsOnly is blocked by itself"), DefaultsOnlyBlocker && DefaultsOnlyBlocker->GetFName() == TEXT("DefaultsOnly"));

	const FProperty* StructMemberBlocker = FEpithPropertyDataContainer::FindInstanceEditBlocker(InstanceData.FindPropertyHandle(TEXT("DefaultsOnlyStruct.Value")));
	TestTrue(TEXT("A member of an EditDefaultsOnly struct is blocked by the struct"), StructMemberBlocker && StructMemberBlocker->GetFName() == TEXT("DefaultsOnlyStruct"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEpithPropertyDataClaimsTest, "Epith.PropertyData.Claims", EpithTests::Flags)

bool FEpithPropertyDataClaimsTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UEpithTestObject> Object(NewObject<UEpithTestObject>(GetTransientPackage()));
	FEpithPropertyDataContainer PropertyData(Object.Get());

	TestTrue(TEXT("First claim succeeds"), PropertyData.TryClaimPath(TEXT("PlainFloat")));
	TestFalse(TEXT("Second claim of the same path fails"), PropertyData.TryClaimPath(TEXT("PlainFloat")));
	TestFalse(TEXT("A child claim can't take a path a layout element already has"), PropertyData.TryClaimChildPath(TEXT("PlainFloat")));

	TestTrue(TEXT("Child claim"), PropertyData.TryClaimChildPath(TEXT("Numbers[0]")));
	TestTrue(TEXT("Child claim (nested)"), PropertyData.TryClaimChildPath(TEXT("Numbers[0].Inner")));
	TestTrue(TEXT("Child claim with a similar prefix"), PropertyData.TryClaimChildPath(TEXT("NumbersExtra[0]")));
	TestFalse(TEXT("A layout element can't take a path a child row already has"), PropertyData.TryClaimPath(TEXT("Numbers[0]")));

	PropertyData.ReleaseChildClaimsUnder(TEXT("Numbers"));

	TestTrue(TEXT("Released: Numbers[0]"), PropertyData.TryClaimChildPath(TEXT("Numbers[0]")));
	TestTrue(TEXT("Released: Numbers[0].Inner"), PropertyData.TryClaimChildPath(TEXT("Numbers[0].Inner")));
	TestFalse(TEXT("Not released: NumbersExtra[0] only shares a prefix"), PropertyData.TryClaimChildPath(TEXT("NumbersExtra[0]")));
	TestFalse(TEXT("Layout claims are never released by ReleaseChildClaimsUnder"), PropertyData.TryClaimPath(TEXT("PlainFloat")));

	return true;
}

/**
 * Regression test for the slow panel open: a brand-new row generator refreshes once on its first tick, and the old code
 * rebuilt the whole pane (with another new generator) in response, about 20 times over. A new container must settle without
 * reporting a structure change.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEpithPropertyDataNewContainerIsStableTest, "Epith.PropertyData.NewContainerIsStable", EpithTests::Flags)

bool FEpithPropertyDataNewContainerIsStableTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		TStrongObjectPtr<UEpithTestObject> Object;
		TSharedPtr<FEpithPropertyDataContainer> PropertyData;
		int32 StructureChanges = 0;
	};

	TSharedRef<FState> State = MakeShared<FState>();
	State->Object.Reset(NewObject<UEpithTestObject>(GetTransientPackage()));
	State->PropertyData = MakeShared<FEpithPropertyDataContainer>(State->Object.Get());
	State->PropertyData->OnStructureChanged.AddLambda([StatePtr = &State.Get()]() { ++StatePtr->StructureChanges; });

	EpithTests::WaitFrames(10);

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, State]()
	{
		TestEqual(TEXT("A new container reports no structure changes while it settles"), State->StructureChanges, 0);
		State->PropertyData.Reset();
		return true;
	}));

	return true;
}

// ================================================================================================
// Details-panel-style rows
// ================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEpithPropertyRowEditConditionHidesTest, "Epith.PropertyRow.EditConditionHides", EpithTests::Flags)

bool FEpithPropertyRowEditConditionHidesTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		TStrongObjectPtr<UEpithTestObject> Object;
		TSharedPtr<FEpithPropertyDataContainer> PropertyData;
		TSharedPtr<SWidget> Row;
	};

	TSharedRef<FState> State = MakeShared<FState>();
	State->Object.Reset(NewObject<UEpithTestObject>(GetTransientPackage()));
	State->Object->bCondition = false;
	State->PropertyData = MakeShared<FEpithPropertyDataContainer>(State->Object.Get());

	TSharedPtr<IDetailTreeNode> Node = State->PropertyData->FindTreeNode(TEXT("HiddenWhenFalse"));

	if (!TestTrue(TEXT("Row exists"), Node.IsValid()))
	{
		return false;
	}

	State->Row = EpithPropertyRow::Make(Node.ToSharedRef(), State->PropertyData.ToSharedRef());

	// A widget's bound visibility is cached and only re-read when the widget is laid out, so lay it out before each check
	State->Row->SlatePrepass(1.0f);

	// Epith shows EditConditionHides rows greyed out rather than hiding them
	TestTrue(TEXT("Still shown while the condition is false"), State->Row->GetVisibility() == EVisibility::Visible);

	// Flip the condition the way the panel would: through the property handle
	TSharedPtr<IPropertyHandle> ConditionHandle = State->PropertyData->FindPropertyHandle(TEXT("bCondition"));

	if (!TestTrue(TEXT("Condition handle exists"), ConditionHandle.IsValid()))
	{
		return false;
	}

	ConditionHandle->SetValue(true);

	// The row generator updates visibility on its tick
	EpithTests::WaitFrames(5);

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, State]()
	{
		State->Row->SlatePrepass(1.0f);
		TestTrue(TEXT("Shown once the condition is true"), State->Row->GetVisibility() == EVisibility::Visible);
		State->Row.Reset();
		State->PropertyData.Reset();
		return true;
	}));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEpithPropertyRowReadOnlyTest, "Epith.PropertyRow.ReadOnlyOnInstance", EpithTests::Flags)

bool FEpithPropertyRowReadOnlyTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UEpithTestObject> Object(NewObject<UEpithTestObject>(GetTransientPackage()));
	TSharedRef<FEpithPropertyDataContainer> PropertyData = MakeShared<FEpithPropertyDataContainer>(Object.Get());

	// EpithPropertyRow::Make returns a visibility box whose single child is either the row itself or a disabled read-only wrapper
	auto IsRowEnabled = [&](FName Path) -> TOptional<bool>
	{
		TSharedPtr<IDetailTreeNode> Node = PropertyData->FindTreeNode(Path);

		if (!Node.IsValid())
		{
			return {};
		}

		TSharedRef<SWidget> Row = EpithPropertyRow::Make(Node.ToSharedRef(), PropertyData);
		FChildren* Children = Row->GetChildren();

		if (!Children || Children->Num() != 1)
		{
			return {};
		}

		return Children->GetChildAt(0)->IsEnabled();
	};

	const TOptional<bool> PlainEnabled = IsRowEnabled(TEXT("PlainFloat"));
	const TOptional<bool> DefaultsOnlyEnabled = IsRowEnabled(TEXT("DefaultsOnly"));
	const TOptional<bool> DefaultsOnlyStructEnabled = IsRowEnabled(TEXT("DefaultsOnlyStruct"));

	TestTrue(TEXT("EditAnywhere row is enabled"), PlainEnabled.IsSet() && PlainEnabled.GetValue());
	TestTrue(TEXT("EditDefaultsOnly row is read-only on an instance"), DefaultsOnlyEnabled.IsSet() && !DefaultsOnlyEnabled.GetValue());
	TestTrue(TEXT("EditDefaultsOnly struct row is read-only on an instance"), DefaultsOnlyStructEnabled.IsSet() && !DefaultsOnlyStructEnabled.GetValue());

	return true;
}

/** An expanded array must pick up an element added after the panel was drawn, without rebuilding the pane. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEpithPropertyRowArrayFollowsElementsTest, "Epith.PropertyRow.ArrayFollowsElements", EpithTests::Flags)

bool FEpithPropertyRowArrayFollowsElementsTest::RunTest(const FString& Parameters)
{
	struct FState
	{
		TStrongObjectPtr<UEpithTestObject> Object;
		TSharedPtr<FEpithPropertyDataContainer> PropertyData;
		TSharedPtr<SWidget> Row;
		int32 StructureChanges = 0;
	};

	TSharedRef<FState> State = MakeShared<FState>();
	State->Object.Reset(NewObject<UEpithTestObject>(GetTransientPackage()));
	State->Object->Numbers = { 10 };

	TSharedPtr<TSet<FName>> ExpandedPaths = MakeShared<TSet<FName>>();
	ExpandedPaths->Add(TEXT("Numbers"));

	State->PropertyData = MakeShared<FEpithPropertyDataContainer>(State->Object.Get(), ExpandedPaths);
	State->PropertyData->OnStructureChanged.AddLambda([StatePtr = &State.Get()]() { ++StatePtr->StructureChanges; });

	TSharedPtr<IDetailTreeNode> Node = State->PropertyData->FindTreeNode(TEXT("Numbers"));

	if (!TestTrue(TEXT("Array row exists"), Node.IsValid()))
	{
		return false;
	}

	TestTrue(TEXT("The layout claims the array"), State->PropertyData->TryClaimPath(TEXT("Numbers")));

	State->Row = EpithPropertyRow::Make(Node.ToSharedRef(), State->PropertyData.ToSharedRef());

	TSharedPtr<IPropertyHandle> ArrayHandle = State->PropertyData->FindPropertyHandle(TEXT("Numbers"));

	if (!TestTrue(TEXT("Array handle exists"), ArrayHandle.IsValid() && ArrayHandle->AsArray().IsValid()))
	{
		return false;
	}

	// Element rows are keyed by the property editor's own path for them (e.g. "Numbers.Numbers[0]"), so ask for it rather than guess
	TSharedRef<IPropertyHandle> FirstElement = ArrayHandle->AsArray()->GetElement(0);
	const FString FirstElementPath = FirstElement->GeneratePathToProperty();
	const FName SecondElementPath = FName(FirstElementPath.LeftChop(3) + TEXT("[1]"));

	if (!TestTrue(TEXT("Element path ends in [0]"), FirstElementPath.EndsWith(TEXT("[0]"))))
	{
		return false;
	}

	// Child rows claim their paths when drawn, so a failed claim means the row for that element exists
	TestFalse(TEXT("Expanded array drew its first element"), State->PropertyData->TryClaimChildPath(FName(FirstElementPath)));

	ArrayHandle->AsArray()->AddItem();

	EpithTests::WaitFrames(5);

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, State, SecondElementPath]()
	{
		TestEqual(TEXT("The array element was added"), State->Object->Numbers.Num(), 2);
		TestFalse(TEXT("The expanded array drew the new element"), State->PropertyData->TryClaimChildPath(SecondElementPath));
		TestEqual(TEXT("Adding an element didn't need a full rebuild"), State->StructureChanges, 0);
		State->Row.Reset();
		State->PropertyData.Reset();
		return true;
	}));

	return true;
}

// ================================================================================================
// Property layout element
// ================================================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEpithPropertyElementWarningsTest, "Epith.PropertyElement.Warnings", EpithTests::Flags)

bool FEpithPropertyElementWarningsTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UEpithTestObject> Object(NewObject<UEpithTestObject>(GetTransientPackage()));
	TSharedRef<FEpithPropertyDataContainer> PropertyData = MakeShared<FEpithPropertyDataContainer>(Object.Get());

	const FEpithWindowElement_Property Missing = FEpithTestAccess::MakePropertyElement(TEXT("DoesNotExist"));
	TestTrue(TEXT("A missing path draws a warning"), EpithTests::GetText(Missing.Draw(FEpithDrawContext(PropertyData))).Contains(TEXT("Missing property")));

	const FEpithWindowElement_Property Empty = FEpithTestAccess::MakePropertyElement(NAME_None);
	TestFalse(TEXT("An unset path draws nothing"), Empty.Draw(FEpithDrawContext(PropertyData)).IsValid());

	const FEpithWindowElement_Property Plain = FEpithTestAccess::MakePropertyElement(TEXT("PlainFloat"));
	const TSharedPtr<SWidget> First = Plain.Draw(FEpithDrawContext(PropertyData));
	const TSharedPtr<SWidget> Second = Plain.Draw(FEpithDrawContext(PropertyData));

	TestTrue(TEXT("The first element for a path draws the row"), First.IsValid() && EpithTests::GetText(First).IsEmpty());
	TestTrue(TEXT("A second element for the same path draws a warning"), EpithTests::GetText(Second).Contains(TEXT("already shown")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
