#pragma once

#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"

class SEpithViewportObjectPane : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SEpithViewportObjectPane) {}

		SLATE_ARGUMENT(UObject*, Target)

		SLATE_ARGUMENT(UEpithPanelDefinition*, PanelDefinition)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	~SEpithViewportObjectPane();

protected:
	/** Kept alive while the pane is open. The layout tree is read from it on every Build() and never cached, so editing the asset can't leave us with a dangling pointer. */
	TStrongObjectPtr<UEpithPanelDefinition> PanelDefinition;

	TWeakObjectPtr<UObject> Target;

	TSharedPtr<FEpithPropertyDataContainer> PropertyData;

protected:
	void OnObjectsReinstanced(const TMap<UObject*, UObject*>& ReplacementMap);

	void OnObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);

	void Build();
};
