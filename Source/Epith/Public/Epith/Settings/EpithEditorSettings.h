#pragma once

#include "Engine/DeveloperSettings.h"

#include "EpithEditorSettings.generated.h"

/** Epith's per-user editor preferences (Editor Preferences > Plugins > Epith). */
UCLASS(Config = EditorPerProjectUserSettings, meta = (DisplayName = "Epith"))
class EPITH_API UEpithEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UEpithEditorSettings();

	static const UEpithEditorSettings* Get() { return GetDefault<UEpithEditorSettings>(); }
	
	virtual FName GetContainerName() const override;

	/** Turns Epith off in the level editor: double-clicking an actor doesn't open a panel, and no hover hint is shown. */
	UPROPERTY(Config, EditAnywhere, Category = "General")
	bool bDisableEpith = false;

	/** Don't show the "Double-click to edit" hint when the cursor rests on an actor that has an Epith panel. */
	UPROPERTY(Config, EditAnywhere, Category = "Hover Hint", meta = (EditCondition = "!bDisableEpith"))
	bool bDisableHoverHint = false;

	/** How long the cursor has to rest on an actor before the hint appears. */
	UPROPERTY(Config, EditAnywhere, Category = "Hover Hint", meta = (EditCondition = "!bDisableEpith && !bDisableHoverHint", ClampMin = 0.0, ClampMax = 1.0, UIMin = 0.0, UIMax = 1.0, Units = "s"))
	float HoverHintDelay = 0.1f;

	bool IsEpithEnabled() const { return !bDisableEpith; }

	bool IsHoverHintEnabled() const { return !bDisableEpith && !bDisableHoverHint; }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
