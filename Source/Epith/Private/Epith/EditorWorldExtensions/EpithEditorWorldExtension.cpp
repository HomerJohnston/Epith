#include "Epith/EditorWorldExtensions/EpithEditorWorldExtension.h"

#include "IAssetViewport.h"
#include "LevelEditor.h"
#include "SLevelViewport.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Epith/EpithLog.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/Widgets/SEpithViewportMasterPane.h"
#include "Epith/Widgets/SEpithViewportObjectPane.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Widgets/Layout/SConstraintCanvas.h"

void UEpithEditorWorldExtension::Init()
{
	SetActive(true);
}

bool UEpithEditorWorldExtension::InputKey(FEditorViewportClient* InViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (Key != EKeys::LeftMouseButton || Event != IE_DoubleClick)
	{
		return false;
	}

	UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();

	if (!EditorActorSubsystem)
	{
		return false;
	}

	TArray<AActor*> SelectedActors = EditorActorSubsystem->GetSelectedLevelActors();

	if (SelectedActors.Num() != 1)
	{
		return false;
	}

	// Only react to double-clicks in level editor viewports, and show the panel in the viewport that was clicked.
	TSharedPtr<IAssetViewport> ClickedViewport = FindLevelViewport(InViewportClient);

	if (!ClickedViewport.IsValid())
	{
		return false;
	}

	return TryShowPanel(SelectedActors[0], ClickedViewport);
}

TArray<FEpithObjectPanelMatch> UEpithEditorWorldExtension::FindPanelMatches(const TArray<UObject*>& Objects, const TArray<UEpithPanelDefinition*>& PanelDefinitions)
{
	TArray<FEpithObjectPanelMatch> Matches;

	for (UObject* Object : Objects)
	{
		if (!IsValid(Object))
		{
			continue;
		}

		const UClass* ObjectClass = Object->GetClass();

		UEpithPanelDefinition* BestDefinition = nullptr;
		const UClass* BestClass = nullptr;

		for (UEpithPanelDefinition* PanelDefinition : PanelDefinitions)
		{
			const UClass* TargetClass = PanelDefinition->GetTargetClass();

			if (!TargetClass || !PanelDefinition->HasRoot() || !ObjectClass->IsChildOf(TargetClass))
			{
				continue;
			}

			if (!BestClass || TargetClass->IsChildOf(BestClass))
			{
				if (BestClass == TargetClass)
				{
					UE_LOG(LogEpith, Warning, TEXT("Panel definitions [%s] and [%s] both target class [%s]; using [%s]."),
						*BestDefinition->GetPathName(), *PanelDefinition->GetPathName(), *TargetClass->GetName(), *BestDefinition->GetPathName());
					continue;
				}

				BestDefinition = PanelDefinition;
				BestClass = TargetClass;
			}
		}

		if (BestDefinition)
		{
			Matches.Emplace(Object, BestDefinition);
		}
	}

	return Matches;
}

bool UEpithEditorWorldExtension::TryShowPanel(UObject* Target, TSharedPtr<IAssetViewport> Viewport)
{
	if (!IsValid(Target) || !Viewport.IsValid())
	{
		return false;
	}
	
	TSharedRef<SConstraintCanvas> Canvas = SNew(SConstraintCanvas);
	
	TSharedRef<SEpithViewportMasterPane> MasterPane = SNew(SEpithViewportMasterPane)
		.Canvas(Canvas)
		.Viewport(Viewport)
		.Target(Target);
	
	if (!MasterPane->HasPanes())
	{
		return false;
	}
	
	TWeakPtr<SEpithViewportMasterPane> WeakMasterPane = MasterPane;
	
	Canvas->AddSlot()
	.Alignment(FVector2D(0, 0))
	.Anchors(FAnchors(0, 0))
	.Offset_Lambda([WeakMasterPane]()
	{
		TSharedPtr<SEpithViewportMasterPane> Pinned = WeakMasterPane.Pin();
		return Pinned.IsValid() ? Pinned->GetOffset() : FMargin(0);
	})
	.AutoSize(true)
	[
		MasterPane
	];
	
	Viewport->AddOverlayWidget(Canvas);
	
	return true;
}

TArray<TSharedRef<SEpithViewportObjectPane>> UEpithEditorWorldExtension::CreateObjectPanes(UObject* Target)
{
	TArray<TSharedRef<SEpithViewportObjectPane>> Panes;
	
	if (!IsValid(Target))
	{
		return Panes;
	}
	
	TArray<UEpithPanelDefinition*> PanelDefinitions = GetPanelDefinitions();
	
	if (PanelDefinitions.Num() == 0)
	{
		return Panes;
	}
	
	TArray<UObject*> Objects = GetAllObjects(Target);
	TArray<FEpithObjectPanelMatch> Matches = FindPanelMatches(Objects, PanelDefinitions);
	
	// Keep the target itself first, then sort the rest by name.
	Matches.StableSort([Target](const FEpithObjectPanelMatch& First, const FEpithObjectPanelMatch& Second)
	{
		if ((First.Object == Target) != (Second.Object == Target))
		{
			return First.Object == Target;
		}
		
		return First.Object->GetName() < Second.Object->GetName();
	});
	
	for (const FEpithObjectPanelMatch& Match : Matches)
	{
		Panes.Add(SNew(SEpithViewportObjectPane)
			.Target(Match.Object)
			.PanelDefinition(Match.PanelDefinition));
	}
	
	return Panes;
}

TSharedPtr<IAssetViewport> UEpithEditorWorldExtension::FindLevelViewport(const FEditorViewportClient* ViewportClient)
{
	if (!ViewportClient)
	{
		return nullptr;
	}

	static const FName LevelEditorName("LevelEditor");

	FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>(LevelEditorName);

	if (!LevelEditorModule)
	{
		return nullptr;
	}

	TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule->GetFirstLevelEditor();

	if (!LevelEditor.IsValid())
	{
		return nullptr;
	}

	for (const TSharedPtr<SLevelViewport>& LevelViewport : LevelEditor->GetViewports())
	{
		if (LevelViewport.IsValid() && &LevelViewport->GetAssetViewportClient() == ViewportClient)
		{
			return LevelViewport;
		}
	}

	return nullptr;
}

TArray<UEpithPanelDefinition*> UEpithEditorWorldExtension::GetPanelDefinitions()
{
	TArray<UEpithPanelDefinition*> PanelDefinitions;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	FARFilter Filter;
	Filter.ClassPaths.Add(UEpithPanelDefinition::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> OutAssets;
	AssetRegistry.GetAssets(Filter, OutAssets);

	for (const FAssetData& AssetData : OutAssets)
	{
		if (UEpithPanelDefinition* PanelDefinition = Cast<UEpithPanelDefinition>(AssetData.GetAsset()))
		{
			PanelDefinitions.Add(PanelDefinition);
		}
	}

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
