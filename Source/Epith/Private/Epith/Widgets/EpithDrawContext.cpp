#include "Epith/Widgets/EpithDrawContext.h"

#include "CoreGlobals.h"
#include "Epith/Widgets/EpithWindowElement.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SWidget.h"

TSharedPtr<SWidget> EpithDraw::DrawSlot(const TInstancedStruct<FEpithWindowElement>& Slot, const FEpithDrawContext& Context, const FEpithSlotActions& Actions)
{
	if (!Context.IsDesigning())
	{
		return Slot.IsValid() ? Slot.Get<FEpithWindowElement>().Draw(Context) : nullptr;
	}

	// The designer edits the panel definition's own layout data, which elements only see as const while drawing.
	// Every edit is followed by a redraw, so these references never outlive the data they point at.
	TInstancedStruct<FEpithWindowElement>& MutableSlot = const_cast<TInstancedStruct<FEpithWindowElement>&>(Slot);

	if (!Slot.IsValid())
	{
		return Context.Designer->MakeEmptySlot(MutableSlot, Actions);
	}

	TSharedPtr<SWidget> ElementWidget = Slot.Get<FEpithWindowElement>().Draw(Context);

	return Context.Designer->WrapElement(MutableSlot, ElementWidget.IsValid() ? ElementWidget.ToSharedRef() : SNullWidget::NullWidget, Actions);
}

// ------------------------------------------------------------------------------------------------

uint64 FEpithColumnLayout::Generation = 0;

FEpithColumnLayout::FEpithColumnLayout(float InFixedNameWidth, float InFixedValueWidth, const FEpithColumnSettings* InSourceSettings)
	: FixedNameWidth(InFixedNameWidth)
	, FixedValueWidth(InFixedValueWidth)
	, SourceSettings(InSourceSettings)
{
}

void FEpithColumnLayout::RegisterNameArea(const TSharedRef<SWidget>& NameArea)
{
	NameAreas.Add(NameArea);
}

FOptionalSize FEpithColumnLayout::GetNameWidth() const
{
	if (FixedNameWidth > 0.0f)
	{
		return FixedNameWidth;
	}

	if (CachedFrame != GFrameCounter || CachedGeneration != Generation)
	{
		CachedFrame = GFrameCounter;
		CachedGeneration = Generation;
		CachedNameWidth = 0.0f;

		for (const TWeakPtr<SWidget>& WeakNameArea : NameAreas)
		{
			TSharedPtr<SWidget> NameArea = WeakNameArea.Pin();

			if (!NameArea.IsValid())
			{
				continue;
			}

			// Skip rows that are hidden (edit condition, collapsed parent): their cached sizes are stale and shouldn't widen the column
			bool bVisible = true;

			for (TSharedPtr<SWidget> Widget = NameArea; Widget.IsValid(); Widget = Widget->GetParentWidget())
			{
				if (!Widget->GetVisibility().IsVisible())
				{
					bVisible = false;
					break;
				}
			}

			if (bVisible)
			{
				// Desired sizes come from Slate's layout prepass; rows measured later this frame use last frame's size, which settles at once
				CachedNameWidth = FMath::Max(CachedNameWidth, static_cast<float>(NameArea->GetDesiredSize().X));
			}
		}
	}

	return CachedNameWidth > 0.0f ? FOptionalSize(CachedNameWidth) : FOptionalSize();
}

void FEpithColumnLayout::MeasureNow(const TSharedRef<SWidget>& NewContent)
{
	// One layout pass caches every new row's desired size. The column widths it produces are thrown away (some rows were
	// measured after the columns were sized); the real pass this frame works them out again from the complete measurements.
	NewContent->SlatePrepass();
	++Generation;
}

FOptionalSize FEpithColumnLayout::GetValueWidth() const
{
	return FixedValueWidth > 0.0f ? FOptionalSize(FixedValueWidth) : FOptionalSize();
}

FEpithDrawContext FEpithDrawContext::WithColumns(const FEpithColumnSettings& Settings) const
{
	FEpithDrawContext ChildContext = *this;
	ChildContext.Columns = MakeShared<FEpithColumnLayout>(Settings.NameColumnWidth, Settings.ValueColumnWidth, &Settings);
	return ChildContext;
}
