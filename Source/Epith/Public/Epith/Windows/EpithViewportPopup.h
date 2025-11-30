#pragma once

#include "Widgets/SUserWidget.h"

struct FEpithPropertyDataContainer;
struct FEpithWindowElement;
class UBangoTargetCollector;
class UBangoTargetGroup;
class AEpith;

#define LOCTEXT_NAMESPACE "Epith"

class FEpithViewportPopup
{
public:
	static bool Show(AActor* Target, const FEpithWindowElement& RootWindowElement);
};

#undef LOCTEXT_NAMESPACE