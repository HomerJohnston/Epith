#pragma once

#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "Epith/EpithPanelDefinition.h"
#include "Epith/Misc/EpithPropertyDataContainer.h"
#include "Epith/Widgets/EpithDrawContext.h"

namespace EpithViewportObjectPane
{
	/** The ribbon behind a section's header (and the panel designer's title): a medium gray, #404040. */
	inline const FLinearColor HeaderRibbonColor = FLinearColor::FromSRGBColor(FColor(64, 64, 64));

	/** The same ribbon while hovered (it's clicked to collapse or expand the section): #4C4C4C. */
	inline const FLinearColor HeaderRibbonHoverColor = FLinearColor::FromSRGBColor(FColor(76, 76, 76));
}

class SEpithViewportObjectPane : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SEpithViewportObjectPane)
		: _bReadOnly(false)
		{}

		SLATE_ARGUMENT(UObject*, Target)

		SLATE_ARGUMENT(UEpithPanelDefinition*, PanelDefinition)

		/** Look-only: nothing in the section can change Target (the designer's REAL VIEW, which shows the class defaults). */
		SLATE_ARGUMENT(bool, bReadOnly)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	~SEpithViewportObjectPane();

	/** False once the target has been destroyed (deleted, unloaded, or a construction-script component that was recreated). */
	bool HasValidTarget() const { return Target.IsValid(); }

	/**
	 * Whether the section's content is showing. Only a Foldable panel definition's section can be collapsed (by clicking its
	 * header); it's expanded unless the user collapsed it,
	 * remembered for this editor session per panel definition and target, so it survives the panel being rebuilt or reopened.
	 */
	bool IsExpanded() const;

	void SetExpanded(bool bExpanded);

protected:
	/** Kept alive while the pane is open. The layout tree is read from it on every Build() and never cached, so editing the asset can't leave us with a dangling pointer. */
	TStrongObjectPtr<UEpithPanelDefinition> PanelDefinition;

	TWeakObjectPtr<UObject> Target;

	bool bReadOnly = false;

	TSharedPtr<FEpithPropertyDataContainer> PropertyData;

	/** Which struct/array rows are expanded. Shared with each rebuilt PropertyData so expansion survives rebuilds. */
	TSharedRef<TSet<FName>> ExpandedPaths = MakeShared<TSet<FName>>();

	/** Which tabs and foldouts are showing, restored when the pane redraws itself. */
	TSharedRef<FEpithViewState> ViewState = MakeShared<FEpithViewState>();

	bool bRebuildPending = false;

	/** Guards against a customization that refreshes its rows every frame, which would otherwise rebuild us forever. */
	int32 RapidRebuildCount = 0;
	double LastAutoRebuildTime = 0.0;

protected:
	void OnObjectsReinstanced(const TMap<UObject*, UObject*>& ReplacementMap);

	void OnObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);

	/** The row generator replaced its top-level rows, so everything we drew is stale. Rebuild on the next Slate tick. */
	void RequestRebuild();

	EActiveTimerReturnType HandleDeferredRebuild(double InCurrentTime, float InDeltaTime);

	/** Makes a new property container for Target and draws the layout. */
	void Build();

	/** Draws the layout from the current property container. */
	void Draw();
};
