#pragma once

#include "Widgets/SUserWidget.h"

struct FEpithClassDefinition;
class UBangoTargetCollector;
class UBangoTargetGroup;
class AEpith;

#define LOCTEXT_NAMESPACE "Epith"

class FEpithViewportPopup
{
public:
	static void Show(AActor* Target, FEpithClassDefinition* AttributeSet);

	static TSharedRef<SWidget> BuildProperties(AActor* Target, FEpithClassDefinition* AttributeSet);
	
	static TSharedRef<SWidget> BuildTabGroup();

	void Test(TSharedRef<SDockTab> DockTab, ETabActivationCause TabActivationCause);
	
	static TSharedRef<SWidget> BuildTabHeader(TArray<FText> Tabs);
	
	static TSharedRef<SWidget> BuildTabPane();
	
	static TSharedRef<SWidget> BuildBoxGroup();
	
	static TSharedRef<SWidget> BuildFoldoutGroup();
	
	
	static TSharedRef<SWidget> BuildDummyPane(FText Text);
};

#undef LOCTEXT_NAMESPACE