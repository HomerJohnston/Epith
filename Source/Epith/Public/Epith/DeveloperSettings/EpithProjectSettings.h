#pragma once

#include "StructUtils/InstancedStruct.h"
#include "Epith/Widgets/EpithWindowElement_VerticalGroup.h"

#include "EpithProjectSettings.generated.h"

UCLASS(Config = Editor, Category = "Epith")
class UEpithProjectSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(Config, EditAnywhere, Category = "Epith", meta = (ExcludeBaseStruct))
	TMap<TSoftClassPtr<AActor>, TInstancedStruct<FEpithWindowElement>> PanelDefinitions;
	
public:
	const TInstancedStruct<FEpithWindowElement>* GetPanelRoot(AActor* Actor) const;
};
