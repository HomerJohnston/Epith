#pragma once

#include "EditorWorldExtension.h"

#include "EpithEditorWorldExtension.generated.h"

class IAssetViewport;
class SEpithViewportObjectPane;
class UEpithPanelDefinition;

struct FEpithObjectPanelMatch
{
	FEpithObjectPanelMatch(UObject* InObject, UEpithPanelDefinition* InPanelDefinition)
		: Object(InObject), PanelDefinition(InPanelDefinition)
	{
	}

	UObject* Object;
	UEpithPanelDefinition* PanelDefinition;
};

UCLASS()
class UEpithEditorWorldExtension : public UEditorWorldExtension
{
	GENERATED_BODY()

public:
	void Init() override;

	bool InputKey(FEditorViewportClient* InViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override;

public:
	/** For each object, finds the panel definition with the most-derived TargetClass that the object's class is a child of. Objects with no match are skipped. */
	static TArray<FEpithObjectPanelMatch> FindPanelMatches(const TArray<UObject*>& Objects, const TArray<UEpithPanelDefinition*>& PanelDefinitions);

	/** Builds and shows a panel for Target (and its subobjects) as an overlay on the given viewport. Returns false if nothing was shown. */
	static bool TryShowPanel(UObject* Target, TSharedPtr<IAssetViewport> Viewport);
	
	/** Creates one object pane per object (Target and its subobjects) that has a matching panel definition. Target's own pane comes first. */
	static TArray<TSharedRef<SEpithViewportObjectPane>> CreateObjectPanes(UObject* Target);

	/** Finds the level editor viewport that is driven by the given viewport client. Returns null if the client doesn't belong to a level viewport. */
	static TSharedPtr<IAssetViewport> FindLevelViewport(const FEditorViewportClient* ViewportClient);

	static TArray<UEpithPanelDefinition*> GetPanelDefinitions();

	static TArray<UObject*> GetAllObjects(UObject* Target);
};
