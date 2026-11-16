#include "Epith/Controls/EpithSpecialControl_AddComponent.h"

#include "Editor.h"
#include "LevelEditor.h"
#include "ScopedTransaction.h"
#include "SubobjectData.h"
#include "SubobjectDataSubsystem.h"
#include "Components/SceneComponent.h"
#include "Epith/EpithLog.h"
#include "GameFramework/Actor.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "Epith"

TSharedPtr<SWidget> FEpithSpecialControl_AddComponent::Draw(const FEpithControlContext& Context) const
{
	const TSubclassOf<UActorComponent> Class = ComponentClass;

	const FText Label = !ButtonText.IsEmpty()
		? ButtonText
		: Class
			? FText::Format(LOCTEXT("AddComponent_Label", "Add {0}"), Class->GetDisplayNameText())
			: LOCTEXT("AddComponent_NoClass", "Add Component (no class set)");

	const bool bAllowMore = bAllowDuplicates;

	return SNew(SButton)
		.Text(Label)
		.HAlign(HAlign_Center)
		.IsEnabled_Lambda([Context, Class, bAllowMore]()
		{
			if (Context.IsPreview() || !Class)
			{
				return false;
			}

			AActor* Actor = Context.GetActor();
			return Actor && (bAllowMore || !Actor->FindComponentByClass(Class));
		})
		.ToolTipText_Lambda([Context, Class, bAllowMore]()
		{
			if (!Class)
			{
				return LOCTEXT("AddComponent_Tip_NoClass", "No component class is set for this button.");
			}

			AActor* Actor = Context.GetActor();

			if (Actor && !bAllowMore && Actor->FindComponentByClass(Class))
			{
				return FText::Format(LOCTEXT("AddComponent_Tip_Exists", "This actor already has a {0}."), Class->GetDisplayNameText());
			}

			return FText::Format(LOCTEXT("AddComponent_Tip", "Add a {0} to this actor."), Class->GetDisplayNameText());
		})
		.OnClicked_Lambda([Context, Class]()
		{
			FText FailReason;

			if (!AddComponent(Context.GetActor(), Class, FailReason))
			{
				UE_LOG(LogEpith, Warning, TEXT("Couldn't add component: %s"), *FailReason.ToString());
			}

			return FReply::Handled();
		});
}

UActorComponent* FEpithSpecialControl_AddComponent::AddComponent(AActor* Actor, TSubclassOf<UActorComponent> ComponentClass, FText& OutFailReason)
{
	if (!IsValid(Actor) || !ComponentClass)
	{
		OutFailReason = LOCTEXT("AddComponent_Fail_Invalid", "No actor or no component class.");
		return nullptr;
	}

	USubobjectDataSubsystem* SubobjectData = USubobjectDataSubsystem::Get();

	if (!SubobjectData)
	{
		OutFailReason = LOCTEXT("AddComponent_Fail_Subsystem", "The subobject data subsystem isn't available.");
		return nullptr;
	}

	// The same path as the Details panel's "Add" button for an actor in the level
	TArray<FSubobjectDataHandle> Handles;
	SubobjectData->GatherSubobjectData(Actor, Handles);

	if (Handles.Num() == 0)
	{
		OutFailReason = LOCTEXT("AddComponent_Fail_NoHandles", "Couldn't read the actor's components.");
		return nullptr;
	}

	const FSubobjectDataHandle ActorHandle = Handles[0];

	FAddNewSubobjectParams Params;
	Params.NewClass = ComponentClass;
	Params.BlueprintContext = nullptr;

	// Scene components go under the root component; others belong to the actor directly
	const FSubobjectDataHandle SceneRoot = ComponentClass->IsChildOf<USceneComponent>() ? SubobjectData->FindSceneRootForSubobject(ActorHandle) : FSubobjectDataHandle::InvalidHandle;
	Params.ParentHandle = SceneRoot.IsValid() ? SceneRoot : ActorHandle;

	FSubobjectDataHandle NewHandle;

	{
		const FScopedTransaction Transaction(FText::Format(LOCTEXT("AddComponent_Transaction", "Add {0}"), ComponentClass->GetDisplayNameText()));

		Actor->Modify();
		NewHandle = SubobjectData->AddNewSubobject(Params, OutFailReason);
	}

	const FSubobjectData* NewData = NewHandle.GetData();

	if (!NewHandle.IsValid() || !NewData)
	{
		return nullptr;
	}

	// The new component itself. FSubobjectData only gives out const pointers publicly; this is the actor's own new
	// component, so changing it is fine.
	const UActorComponent* FoundComponent = NewData->FindComponentInstanceInActor(Actor);

	if (!FoundComponent)
	{
		FoundComponent = NewData->GetObject<UActorComponent>();
	}

	UActorComponent* NewComponent = const_cast<UActorComponent*>(FoundComponent);

	if (!NewComponent)
	{
		OutFailReason = LOCTEXT("AddComponent_Fail_NotFound", "The component was added, but couldn't be found afterwards.");
		return nullptr;
	}

	// Let the Details panel and viewports show the new component
	if (FLevelEditorModule* LevelEditor = FModuleManager::GetModulePtr<FLevelEditorModule>("LevelEditor"))
	{
		LevelEditor->BroadcastComponentsEdited();
	}

	if (GEditor)
	{
		GEditor->RedrawLevelEditingViewports();
	}

	return NewComponent;
}

#undef LOCTEXT_NAMESPACE
