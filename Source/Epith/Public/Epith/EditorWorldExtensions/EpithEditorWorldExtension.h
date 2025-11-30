#pragma once

#include "EditorWorldExtension.h"
#include "StructUtils/InstancedStruct.h"

#include "EpithEditorWorldExtension.generated.h"

struct FEpithWindowElement;

UCLASS()
class UEpithEditorWorldExtension : public UEditorWorldExtension
{
	GENERATED_BODY()
	
public:
	void Init() override;
	
	bool InputKey(FEditorViewportClient* InViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override;
	
public:
	const TInstancedStruct<FEpithWindowElement>* GetPanelRoot(AActor* Actor) const;
};