#include "EpithPanelDefinitionEditor.h"

#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "SEpithPanelDesigner.h"
#include "Epith/EpithPanelDefinition.h"
#include "Framework/Docking/TabManager.h"
#include "Styling/AppStyle.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "EpithPanelDefinitionEditor"

const FName FEpithPanelDefinitionEditor::DetailsTabId(TEXT("EpithPanelDefinitionEditor_Details"));
const FName FEpithPanelDefinitionEditor::DesignerTabId(TEXT("EpithPanelDefinitionEditor_Designer"));

void FEpithPanelDefinitionEditor::Init(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UEpithPanelDefinition* InDefinition)
{
	Definition = InDefinition;

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	DetailsView = PropertyEditor.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(InDefinition);

	TWeakPtr<IDetailsView> WeakDetailsView = DetailsView;

	Designer = SNew(SEpithPanelDesigner, InDefinition)
		.OnDefinitionEdited_Lambda([WeakDetailsView]()
		{
			// The designer can change which element types live in the layout; the Details view has to rebuild for that
			if (TSharedPtr<IDetailsView> PinnedDetailsView = WeakDetailsView.Pin())
			{
				PinnedDetailsView->ForceRefresh();
			}
		});

	// Details on the left, designer on the right. Later the designer could become a tab on top of the Details view.
	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("Standalone_EpithPanelDefinitionEditor_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.4f)
				->AddTab(DetailsTabId, ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.6f)
				->AddTab(DesignerTabId, ETabState::OpenedTab)
			)
		);

	InitAssetEditor(Mode, InitToolkitHost, TEXT("EpithPanelDefinitionEditorApp"), Layout, true, true, InDefinition);
}

FName FEpithPanelDefinitionEditor::GetToolkitFName() const
{
	return TEXT("EpithPanelDefinitionEditor");
}

FText FEpithPanelDefinitionEditor::GetBaseToolkitName() const
{
	return LOCTEXT("BaseToolkitName", "Epith Panel Editor");
}

FString FEpithPanelDefinitionEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("Epith ");
}

FLinearColor FEpithPanelDefinitionEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

void FEpithPanelDefinitionEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenuCategory", "Epith Panel Editor"));

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FEpithPanelDefinitionEditor::SpawnDetailsTab))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(DesignerTabId, FOnSpawnTab::CreateSP(this, &FEpithPanelDefinitionEditor::SpawnDesignerTab))
		.SetDisplayName(LOCTEXT("DesignerTab", "Panel Designer"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit"));
}

void FEpithPanelDefinitionEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(DetailsTabId);
	InTabManager->UnregisterTabSpawner(DesignerTabId);
}

TSharedRef<SDockTab> FEpithPanelDefinitionEditor::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTab", "Details"))
		[
			DetailsView.ToSharedRef()
		];
}

TSharedRef<SDockTab> FEpithPanelDefinitionEditor::SpawnDesignerTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("DesignerTab", "Panel Designer"))
		[
			Designer.ToSharedRef()
		];
}

#undef LOCTEXT_NAMESPACE
