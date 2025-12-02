#include "Epith/EditorWorldExtensions/EpithEditorWorldExtension.h"

#include "IAssetViewport.h"
#include "LevelEditor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/DeveloperSettings/EpithProjectSettings.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"
#include "Epith/Style/EpithColor.h"
#include "Epith/Subsystems/EpithEditorSubsystem.h"
#include "Epith/Widgets/SEpithViewportMasterPane.h"
#include "Epith/Widgets/SEpithViewportObjectPane.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Widgets/Layout/SConstraintCanvas.h"

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

		return TryShowPanel(Actor);
	}

	return false;
}

TArray<FEpithObjectWindowRootMatch> UEpithEditorWorldExtension::GetPanelRoots(const TArray<UObject*>& Objects, const TArray<const UEpithPanelDefinition*>& PanelDefinitions)
{
	TArray<FEpithObjectWindowRootMatch> Matches;
	
	for (UObject* Object : Objects)
	{
		if (!IsValid(Object))
		{
			continue;
		}
		
		for (const UEpithPanelDefinition* PanelDefinition : PanelDefinitions)
		{
			if (!PanelDefinition->HasRoot())
			{
				continue;
			}
			
			if (Object->GetClass()->IsChildOf(PanelDefinition->GetTargetActorType()))
			{
				Matches.Add( { Object, &PanelDefinition->GetRoot()->Get<FEpithWindowElement>() } );
			}
		}
	}
	
	return Matches;
}

bool UEpithEditorWorldExtension::TryShowPanel(UObject* Target)
{
	if (!IsValid(Target))
	{
		return false;
	}
	
	TArray<const UEpithPanelDefinition*> PanelDefinitions = GetPanelDefinitions();
	
	if (PanelDefinitions.Num() == 0)
	{
		return false;
	}
	
	TArray<UObject*> Objects = GetAllObjects(Target);
	TArray<FEpithObjectWindowRootMatch> ObjectWindowMatches = GetPanelRoots(Objects, PanelDefinitions);
		
	FVector2D ScreenPos = GetInitialScreenPos();
		
	int32 Panels = 0;
	
	TArray<TSharedPtr<SWidget>> Panes;
	
	for (const FEpithObjectWindowRootMatch& Match : ObjectWindowMatches)
	{
		UObject* CurrentTarget = Match.Object;
		const FEpithWindowElement* RootWindowElement = Match.WindowRoot;

		TSharedPtr<FEpithPropertyDataContainer> PropertyData = MakeShared<FEpithPropertyDataContainer>(CurrentTarget);
		
		TSharedPtr<SWidget> Widget = SNew(SEpithViewportObjectPane)
			.Target(CurrentTarget)
			.PropertyData(PropertyData)
			.Root(RootWindowElement);
	
		if (!Widget)
		{
			continue;
		}
		
		Panes.Add(Widget);
		
		++Panels;
	}
	
	TSharedRef<SConstraintCanvas> MasterPane = SNew(SConstraintCanvas);
	
	TSharedPtr<SEpithViewportMasterPane> MasterPane_Int = SNew(SEpithViewportMasterPane)
		.Owner(MasterPane);
	
	MasterPane->AddSlot()
	.Alignment(FVector2D(0, 0))
	.Anchors(FAnchors(0, 0))
	.Offset_Lambda([MasterPane_Int] ()
	{
		return FMargin(400, 200, 600, 600);
	})
	.AutoSize(true)
	[
		SNew(SEpithViewportMasterPane)
		.ChildPanes(Panes)
	];
	
	static const FName LevelEditorName("LevelEditor");
	if (FModuleManager::Get().IsModuleLoaded(LevelEditorName))
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(LevelEditorName);
		TSharedPtr<IAssetViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveViewport();
		if (ActiveLevelViewport.IsValid())
		{
			ActiveLevelViewport->AddOverlayWidget(MasterPane_Int.ToSharedRef());
		}
	}
	
	/*
	FSlateApplication::Get().PushMenu(
		FSlateApplication::Get().GetUserFocusedWidget(0).ToSharedRef(),
		FWidgetPath(),
		MasterPane ,
		FDeprecateSlateVector2D(ScreenPos.X, ScreenPos.Y),
		FPopupTransitionEffect(FPopupTransitionEffect::TypeInPopup),
		true);
	*/
	
	return Panels > 0;
}

FVector2D UEpithEditorWorldExtension::GetInitialScreenPos()
{
	return {800, 400};
}

TArray<const UEpithPanelDefinition*> UEpithEditorWorldExtension::GetPanelDefinitions()
{
	TArray<const UEpithPanelDefinition*> PanelDefinitions;
	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	
	FARFilter Filter;
	Filter.ClassPaths.Add(FTopLevelAssetPath(UEpithPanelDefinition::StaticClass()->GetClassPathName()));
	Filter.bRecursiveClasses = true;
	
	TArray<FAssetData> OutAssets;
	AssetRegistry.GetAssets(Filter, OutAssets);
	
	// We anticipate that most projects will have more panel definitions than objects on an actor (this will probably usually be false, but it's a better assumption for larger cases)
	// Iterate the "large" panel definitions array. For each element, try to match it with one of the target objects. 
	
	for (const FAssetData& AssetData : OutAssets)
	{
		const UEpithPanelDefinition* PanelDefinition = Cast<const UEpithPanelDefinition>(AssetData.GetAsset());
		
		if (PanelDefinition)
		{
			PanelDefinitions.Add(PanelDefinition);
		}
	}
	
	// Make it so that the "child-most" classes are always on the left and discovered first when Objects are trying to match to these
	PanelDefinitions.Sort( [] (const UEpithPanelDefinition& Left, const UEpithPanelDefinition& Right)
	{
		return Left.IsA(Right.GetClass());
	});
	
	return PanelDefinitions;
}

TArray<UObject*> UEpithEditorWorldExtension::GetAllObjects(UObject* Target)
{
	TArray<UObject*> Objects;

	if (!Target)
	{
		return Objects;
	}		

	GetObjectsWithOuter(Target, Objects);
	
	Objects.Insert(Target, 0);

	return Objects;
}

