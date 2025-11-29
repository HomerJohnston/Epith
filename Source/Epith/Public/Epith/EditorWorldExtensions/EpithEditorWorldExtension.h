#pragma once

#include "EditorWorldExtension.h"

#include "EpithEditorWorldExtension.generated.h"

UCLASS()
class UEpithEditorWorldExtension : public UEditorWorldExtension
{
	GENERATED_BODY()
	
public:
	void Init() override;
	
	bool InputKey(FEditorViewportClient* InViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override;
};