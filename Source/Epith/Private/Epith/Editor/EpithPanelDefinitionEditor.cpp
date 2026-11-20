#include "EpithPanelDefinitionEditor.h"

#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "SEpithElementDetails.h"
#include "SEpithPanelDesigner.h"
#include "Epith/EpithPanelDefinition.h"
#include "Framework/Docking/TabManager.h"
#include "Styling/AppStyle.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "EpithPanelDefinitionEditor"

const FName FEpithPanelDefinitionEditor::DesignerTabId(TEXT("EpithPanelDefinitionEditor_Designer"));
const FName FEpithPanelDefinitionEditor::TutorialTabId(TEXT("EpithPanelDefinitionEditor_Tutorial"));

void FEpithPanelDefinitionEditor::Init(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UEpithPanelDefinition* InDefinition)
{
	Definition = InDefinition;

	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	// The definition's own settings (Target Class, Title and so on), shown in the designer's PANEL PROPERTIES section. Everything
	// except Root: the layout under it is edited in the designer itself.
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	DetailsView = PropertyEditor.CreateDetailView(DetailsViewArgs);
	DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda([](const FPropertyAndParent& PropertyAndParent)
	{
		static const FName RootName(TEXT("Root"));

		if (PropertyAndParent.Property.GetFName() == RootName && PropertyAndParent.Property.GetOwnerClass() == UEpithPanelDefinition::StaticClass())
		{
			return false;
		}

		for (const FProperty* Parent : PropertyAndParent.ParentProperties)
		{
			if (Parent && Parent->GetFName() == RootName && Parent->GetOwnerClass() == UEpithPanelDefinition::StaticClass())
			{
				return false;
			}
		}

		return true;
	}));
	DetailsView->SetObject(InDefinition);

	ElementDetails = SNew(SEpithElementDetails, InDefinition);

	Designer = SNew(SEpithPanelDesigner, InDefinition)
		.ElementDetails(ElementDetails)
		.PanelDetails(DetailsView);

	// The designer (its side panes hold the panel's settings and the selected element's), with the Tutorial tab beside it.
	// v5: the Tutorial tab was added; a new name so saved layouts without it aren't restored.
	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("Standalone_EpithPanelDefinitionEditor_v5")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(1.0f)
				->AddTab(DesignerTabId, ETabState::OpenedTab)
				->AddTab(TutorialTabId, ETabState::OpenedTab)
				->SetForegroundTab(DesignerTabId)
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

	InTabManager->RegisterTabSpawner(DesignerTabId, FOnSpawnTab::CreateSP(this, &FEpithPanelDefinitionEditor::SpawnDesignerTab))
		.SetDisplayName(LOCTEXT("DesignerTab", "Panel Designer"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit"));

	InTabManager->RegisterTabSpawner(TutorialTabId, FOnSpawnTab::CreateSP(this, &FEpithPanelDefinitionEditor::SpawnTutorialTab))
		.SetDisplayName(LOCTEXT("TutorialTab", "Tutorial"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Help"));
}

void FEpithPanelDefinitionEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(DesignerTabId);
	InTabManager->UnregisterTabSpawner(TutorialTabId);
}

TSharedRef<SDockTab> FEpithPanelDefinitionEditor::SpawnDesignerTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("DesignerTab", "Panel Designer"))
		[
			Designer.ToSharedRef()
		];
}

TSharedRef<SDockTab> FEpithPanelDefinitionEditor::SpawnTutorialTab(const FSpawnTabArgs& Args)
{
	// A placeholder until the tutorial is written: how to build a panel, for someone new to Epith
	return SNew(SDockTab)
		.Label(LOCTEXT("TutorialTab", "Tutorial"))
		[
			SNew(SBox)
			.Padding(16)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TutorialPlaceholder", "Hello World"))
			]
		];
}

#undef LOCTEXT_NAMESPACE
