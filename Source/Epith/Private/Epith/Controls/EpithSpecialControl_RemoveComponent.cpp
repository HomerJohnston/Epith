#include "Epith/Controls/EpithSpecialControl_RemoveComponent.h"

#include "Editor.h"
#include "LevelEditor.h"
#include "ScopedTransaction.h"
#include "SubobjectData.h"
#include "SubobjectDataSubsystem.h"
#include "Epith/EpithLog.h"
#include "GameFramework/Actor.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Input/SButton.h"

#define LOCTEXT_NAMESPACE "Epith"

TSharedPtr<SWidget> FEpithSpecialControl_RemoveComponent::Draw(const FEpithControlContext& Context) const
{
	const TSubclassOf<UActorComponent> Class = ComponentClass;
	const bool bAll = bRemoveAll;
	
	const FText Label = !ButtonText.IsEmpty()
		? ButtonText
		: Class
			? FText::Format(LOCTEXT("RemoveComponent_Label", "Remove {0}"), Class->GetDisplayNameText())
			: LOCTEXT("RemoveComponent_NoClass", "Remove Component (no class set)");
	
	return SNew(SButton)
		.Text(Label)
		.HAlign(HAlign_Center)
		.IsEnabled_Lambda([Context, Class]()
		{
			return !Context.IsPreview() && Class && FindRemovableComponents(Context.GetActor(), Class).Num() > 0;
		})
		.ToolTipText_Lambda([Context, Class, bAll]()
		{
			if (!Class)
			{
				return LOCTEXT("RemoveComponent_Tip_NoClass", "No component class is set for this button.");
			}
			
			AActor* Actor = Context.GetActor();
			
			if (Actor && FindRemovableComponents(Actor, Class).Num() == 0)
			{
				return Actor->FindComponentByClass(Class)
					? FText::Format(LOCTEXT("RemoveComponent_Tip_Fixed", "This actor's {0} is part of its class and can't be removed here."), Class->GetDisplayNameText())
					: FText::Format(LOCTEXT("RemoveComponent_Tip_None", "This actor has no {0}."), Class->GetDisplayNameText());
			}
			
			return bAll
				? FText::Format(LOCTEXT("RemoveComponent_Tip_All", "Remove every {0} from this actor."), Class->GetDisplayNameText())
				: FText::Format(LOCTEXT("RemoveComponent_Tip", "Remove the {0} from this actor."), Class->GetDisplayNameText());
		})
		.OnClicked_Lambda([Context, Class, bAll]()
		{
			AActor* Actor = Context.GetActor();
			TArray<UActorComponent*> Components = FindRemovableComponents(Actor, Class);
			
			if (!bAll && Components.Num() > 1)
			{
				Components.SetNum(1);
			}
			
			FText FailReason;
			
			if (RemoveComponents(Actor, Components, FailReason) < Components.Num())
			{
				UE_LOG(LogEpith, Warning, TEXT("Couldn't remove component: %s"), *FailReason.ToString());
			}
			
			return FReply::Handled();
		});
}

TArray<UActorComponent*> FEpithSpecialControl_RemoveComponent::FindRemovableComponents(AActor* Actor, TSubclassOf<UActorComponent> ComponentClass)
{
	TArray<UActorComponent*> Removable;
	
	if (!IsValid(Actor) || !ComponentClass)
	{
		return Removable;
	}
	
	TArray<UActorComponent*> Components;
	Actor->GetComponents(ComponentClass, Components);
	
	for (UActorComponent* Component : Components)
	{
		// Components added to this actor in the level; the ones from its class (C++ or Blueprint) can't be deleted on an instance
		if (IsValid(Component) && Component->CreationMethod == EComponentCreationMethod::Instance)
		{
			Removable.Add(Component);
		}
	}
	
	return Removable;
}

int32 FEpithSpecialControl_RemoveComponent::RemoveComponents(AActor* Actor, const TArray<UActorComponent*>& Components, FText& OutFailReason)
{
	if (!IsValid(Actor) || Components.Num() == 0)
	{
		OutFailReason = LOCTEXT("RemoveComponent_Fail_Invalid", "No actor or no components to remove.");
		return 0;
	}
	
	USubobjectDataSubsystem* SubobjectData = USubobjectDataSubsystem::Get();
	
	if (!SubobjectData)
	{
		OutFailReason = LOCTEXT("RemoveComponent_Fail_Subsystem", "The subobject data subsystem isn't available.");
		return 0;
	}
	
	// The same path as deleting a component in the Details panel for an actor in the level
	TArray<FSubobjectDataHandle> Handles;
	SubobjectData->GatherSubobjectData(Actor, Handles);
	
	if (Handles.Num() == 0)
	{
		OutFailReason = LOCTEXT("RemoveComponent_Fail_NoHandles", "Couldn't read the actor's components.");
		return 0;
	}
	
	const FSubobjectDataHandle ActorHandle = Handles[0];
	TArray<FSubobjectDataHandle> ToDelete;
	
	for (UActorComponent* Component : Components)
	{
		const FSubobjectDataHandle Handle = SubobjectData->FindHandleForObject(ActorHandle, Component);
		const FSubobjectData* Data = Handle.GetData();
		
		if (Data && Data->CanDelete())
		{
			ToDelete.Add(Handle);
		}
	}
	
	if (ToDelete.Num() < Components.Num())
	{
		OutFailReason = LOCTEXT("RemoveComponent_Fail_CantDelete", "Some components can't be removed (they're part of the actor's class).");
	}
	
	if (ToDelete.Num() == 0)
	{
		return 0;
	}
	
	int32 NumDeleted = 0;
	
	{
		const FScopedTransaction Transaction(ToDelete.Num() == 1
			? LOCTEXT("RemoveComponent_Transaction", "Remove Component")
			: LOCTEXT("RemoveComponents_Transaction", "Remove Components"));
		
		Actor->Modify();
		
		FSubobjectDataHandle ComponentToSelect;
		NumDeleted = SubobjectData->DeleteSubobjects(ActorHandle, ToDelete, ComponentToSelect, nullptr);
	}
	
	// Let the Details panel and viewports catch up
	if (FLevelEditorModule* LevelEditor = FModuleManager::GetModulePtr<FLevelEditorModule>("LevelEditor"))
	{
		LevelEditor->BroadcastComponentsEdited();
	}
	
	if (GEditor)
	{
		GEditor->RedrawLevelEditingViewports();
	}
	
	return NumDeleted;
}

#undef LOCTEXT_NAMESPACE
