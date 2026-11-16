#include "SEpithElementDetails.h"

#include "Editor.h"
#include "IDetailsView.h"
#include "IStructureDetailsView.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/Customizations/PropertyTypeCustomization_EpithPropertySelector.h"
#include "Epith/Misc/EpithPropertySelector.h"
#include "Epith/Widgets/EpithWindowElement.h"
#include "UObject/StructOnScope.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "EpithElementDetails"

namespace EpithElementDetails
{
	/** Child elements are edited by selecting them in the designer, so their properties are hidden here. */
	bool IsPropertyShown(const FPropertyAndParent& PropertyAndParent)
	{
		static const FName ChildElementProperties[] = { TEXT("Slots"), TEXT("Child"), TEXT("Element") };

		const FProperty& Property = PropertyAndParent.Property;
		const UStruct* Owner = Property.GetOwnerStruct();

		if (!Owner || Owner->GetOutermost()->GetFName() != FName(TEXT("/Script/Epith")))
		{
			return true;
		}

		for (const FName Name : ChildElementProperties)
		{
			if (Property.GetFName() == Name)
			{
				return false;
			}
		}

		return true;
	}
}

void SEpithElementDetails::Construct(const FArguments& InArgs, UEpithPanelDefinition* InDefinition)
{
	Definition = InDefinition;

	SlotView = MakeStructView();
	ElementView = MakeStructView();

	ChildSlot
	[
		SNew(SScrollBox)
		+ SScrollBox::Slot()
		.Padding(8, 8, 8, 4)
		[
			SAssignNew(SlotHeading, STextBlock)
			.Font(FAppStyle::GetFontStyle("BoldFont"))
		]
		+ SScrollBox::Slot()
		[
			SlotView->GetWidget().ToSharedRef()
		]
		+ SScrollBox::Slot()
		.Padding(8, 12, 8, 4)
		[
			SAssignNew(ElementHeading, STextBlock)
			.Font(FAppStyle::GetFontStyle("BoldFont"))
		]
		+ SScrollBox::Slot()
		[
			ElementView->GetWidget().ToSharedRef()
		]
	];

	ShowNothing();
}

TSharedRef<IStructureDetailsView> SEpithElementDetails::MakeStructView()
{
	FPropertyEditorModule& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.NotifyHook = this;

	FStructureDetailsViewArgs StructureViewArgs;

	TSharedRef<IStructureDetailsView> View = PropertyEditor.CreateStructureDetailView(DetailsViewArgs, StructureViewArgs, nullptr);

	if (IDetailsView* DetailsView = View->GetDetailsView())
	{
		DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateStatic(&EpithElementDetails::IsPropertyShown));

		// The property picker normally finds its panel definition through the edited object; here there isn't one, so hand it over
		TWeakObjectPtr<UEpithPanelDefinition> WeakDefinition = Definition;

		DetailsView->RegisterInstancedCustomPropertyTypeLayout(
			FEpithPropertySelector::StaticStruct()->GetFName(),
			FOnGetPropertyTypeCustomizationInstance::CreateLambda([WeakDefinition]()
			{
				return FPropertyTypeCustomization_EpithPropertySelector::MakeInstanceFor(WeakDefinition);
			}));
	}

	return View;
}

void SEpithElementDetails::ShowNothing()
{
	SlotView->SetStructureData(nullptr);
	ElementView->SetStructureData(nullptr);

	SlotView->GetWidget()->SetVisibility(EVisibility::Collapsed);
	SlotHeading->SetVisibility(EVisibility::Collapsed);

	ElementView->GetWidget()->SetVisibility(EVisibility::Collapsed);
	ElementHeading->SetText(LOCTEXT("NothingSelected", "Click an element in the Panel Designer to edit its settings here."));
	ElementHeading->SetVisibility(EVisibility::Visible);
}

void SEpithElementDetails::SetSelection(TInstancedStruct<FEpithWindowElement>* Element, const UScriptStruct* SlotStruct, void* SlotMemory)
{
	const UScriptStruct* ElementStruct = Element ? Element->GetScriptStruct() : nullptr;
	void* ElementMemory = Element ? Element->GetMutableMemory() : nullptr;

	if (!Element && !SlotMemory)
	{
		bShowingSomething = false;
		bSuspended = false;
		bOwnChangePending = false;
		ShowNothing();
		return;
	}

	const bool bSame = bShowingSomething && !bSuspended
		&& ShownSlotStruct == SlotStruct && ShownSlotMemory == SlotMemory
		&& ShownElementStruct == ElementStruct && ShownElementMemory == ElementMemory;

	if (bSame)
	{
		// The definition changed but this is still the same data: re-read the values, unless we made the change ourselves
		// (rebuilding then would interrupt editing in this tab)
		if (!bOwnChangePending)
		{
			if (IDetailsView* View = SlotView->GetDetailsView()) { View->ForceRefresh(); }
			if (IDetailsView* View = ElementView->GetDetailsView()) { View->ForceRefresh(); }
		}

		bOwnChangePending = false;
		return;
	}

	bShowingSomething = true;
	bSuspended = false;
	bOwnChangePending = false;
	ShownSlotStruct = SlotStruct;
	ShownSlotMemory = SlotMemory;
	ShownElementStruct = ElementStruct;
	ShownElementMemory = ElementMemory;

	UPackage* Package = Definition.IsValid() ? Definition->GetPackage() : nullptr;

	// Slot settings (only for elements in a group)
	if (SlotStruct && SlotMemory)
	{
		TSharedRef<FStructOnScope> SlotData = MakeShared<FStructOnScope>(SlotStruct, static_cast<uint8*>(SlotMemory));
		SlotData->SetPackage(Package);
		SlotView->SetStructureData(SlotData);

		SlotHeading->SetText(LOCTEXT("SlotHeading", "Slot"));
		SlotHeading->SetVisibility(EVisibility::Visible);
		SlotView->GetWidget()->SetVisibility(EVisibility::Visible);
	}
	else
	{
		SlotView->SetStructureData(nullptr);
		SlotHeading->SetVisibility(EVisibility::Collapsed);
		SlotView->GetWidget()->SetVisibility(EVisibility::Collapsed);
	}

	// The element's settings
	if (ElementStruct && ElementMemory)
	{
		TSharedRef<FStructOnScope> ElementData = MakeShared<FStructOnScope>(ElementStruct, static_cast<uint8*>(ElementMemory));
		ElementData->SetPackage(Package);
		ElementView->SetStructureData(ElementData);

		ElementHeading->SetText(ElementStruct->GetDisplayNameText());
		ElementView->GetWidget()->SetVisibility(EVisibility::Visible);
	}
	else
	{
		ElementView->SetStructureData(nullptr);
		ElementHeading->SetText(LOCTEXT("EmptySlotHeading", "Empty slot (pick a container for it in the designer)"));
		ElementView->GetWidget()->SetVisibility(EVisibility::Collapsed);
	}

	ElementHeading->SetVisibility(EVisibility::Visible);
}

void SEpithElementDetails::Suspend()
{
	if (!bShowingSomething || bSuspended)
	{
		return;
	}

	bSuspended = true;
	SlotView->SetStructureData(nullptr);
	ElementView->SetStructureData(nullptr);
}

void SEpithElementDetails::NotifyPreChange(FProperty* PropertyAboutToChange)
{
	UEpithPanelDefinition* PanelDefinition = Definition.Get();

	if (!PanelDefinition)
	{
		return;
	}

	// Record the definition in the details view's transaction, or in our own if there isn't one
	if (!GUndo && GEditor && !bOwnTransaction)
	{
		GEditor->BeginTransaction(LOCTEXT("EditElementTransaction", "Edit Element"));
		bOwnTransaction = true;
	}

	PanelDefinition->Modify();
}

void SEpithElementDetails::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	// While a value is being dragged, wait for the final value
	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
	{
		return;
	}

	if (UEpithPanelDefinition* PanelDefinition = Definition.Get())
	{
		bApplyingChange = true;
		bOwnChangePending = true;

		// Lets the designer (and the Details tab) know, so the preview redraws
		PanelDefinition->PostEditChange();

		bApplyingChange = false;
	}

	if (bOwnTransaction && GEditor)
	{
		GEditor->EndTransaction();
		bOwnTransaction = false;
	}
}

#undef LOCTEXT_NAMESPACE
