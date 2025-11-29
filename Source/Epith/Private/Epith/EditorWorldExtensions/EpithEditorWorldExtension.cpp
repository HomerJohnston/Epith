#include "Epith/EditorWorldExtensions/EpithEditorWorldExtension.h"

#include "Epith/Subsystems/EpithEditorSubsystem.h"
#include "Epith/Windows/EpithViewportPopup.h"
#include "Subsystems/EditorActorSubsystem.h"

const FName EpithMetaDataKey = TEXT("Epith");

void UEpithEditorWorldExtension::Init()
{
	SetActive(true);
}

bool UEpithEditorWorldExtension::InputKey(FEditorViewportClient* InViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (Event == IE_DoubleClick)
	{
		UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
		TArray<AActor*> SelectedActors;
		
		if (!EditorActorSubsystem)
		{
			return false;
		}

		SelectedActors = EditorActorSubsystem->GetSelectedLevelActors();
		
		if (SelectedActors.Num() != 1)
		{
			return false;
		}

		AActor* Actor = SelectedActors[0];

		const UClass* Class = Actor->GetClass();
		
		// Dig to native class
		bool bFoundEpithAttributeSet = false;
		
		UEpithEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UEpithEditorSubsystem>();
		FEpithClassDefinition* ClassAttributeSet = nullptr;
		
		while (!bFoundEpithAttributeSet)
		{
			ClassAttributeSet = Subsystem->GetClassAttributeSet(Class);
			
			if (ClassAttributeSet)
			{
				bFoundEpithAttributeSet = true;
				break;
			}
			
			Class = Class->GetSuperClass();
			
			if (!Class)
			{
				break;
			}
		}
		
		if (ClassAttributeSet)
		{
			FEpithViewportPopup::Show(Actor, ClassAttributeSet);
			return true;
		}
	}

	return false;
}
