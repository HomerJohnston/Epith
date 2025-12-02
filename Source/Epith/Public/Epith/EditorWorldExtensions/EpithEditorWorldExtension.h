#pragma once

#include "EditorWorldExtension.h"
#include "StructUtils/InstancedStruct.h"

#include "EpithEditorWorldExtension.generated.h"

struct FEpithWindowElement;
class UEpithPanelDefinition;

struct FEpithObjectWindowRootMatch
{
	FEpithObjectWindowRootMatch(UObject* InObject, const FEpithWindowElement* InWindowRoot)
		: Object(InObject), WindowRoot(InWindowRoot)
	{
	}
	
	UObject* Object;
	const FEpithWindowElement* WindowRoot;
};

UCLASS()
class UEpithEditorWorldExtension : public UEditorWorldExtension
{
	GENERATED_BODY()
	
public:
	void Init() override;
	
	bool InputKey(FEditorViewportClient* InViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override;
	
public:
	static TArray<FEpithObjectWindowRootMatch> GetPanelRoots(const TArray<UObject*>& Objects, const TArray<const UEpithPanelDefinition*>& PanelDefinitions);
	
	static bool TryShowPanel(UObject* Target);
	
	static FVector2D GetInitialScreenPos();
	
	static TArray<const UEpithPanelDefinition*> GetPanelDefinitions();
	
	static TArray<UObject*> GetAllObjects(UObject* Target);
};