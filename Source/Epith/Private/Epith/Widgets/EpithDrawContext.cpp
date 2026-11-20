#include "Epith/Widgets/EpithDrawContext.h"

#include "Epith/Widgets/EpithWindowElement_Divider.h"

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

	Context.Designer->BeginSlot(Slot);
	TSharedPtr<SWidget> ElementWidget = Slot.Get<FEpithWindowElement>().Draw(Context);
	Context.Designer->EndSlot();

	return Context.Designer->WrapElement(MutableSlot, ElementWidget.IsValid() ? ElementWidget.ToSharedRef() : SNullWidget::NullWidget, Actions);
}

// ------------------------------------------------------------------------------------------------

uint64 FEpithColumnLayout::Generation = 0;

FEpithColumnLayout::FEpithColumnLayout(float InNameFraction, const FEpithColumnSettings* InSourceSettings)
	: SourceSettings(InSourceSettings)
{
	SetNameFraction(InNameFraction);
}

void FEpithColumnLayout::SetNameFraction(float Fraction)
{
	// Anything saved outside 0..1 (by hand, or an old asset) is brought back into range; 0 stays automatic
	NameFraction = Fraction > 0.0f ? FMath::Clamp(Fraction, MinNameFraction, MaxNameFraction) : 0.0f;
}

void FEpithColumnLayout::RegisterNameArea(const TSharedRef<SWidget>& NameArea)
{
	NameAreas.Add(NameArea);
}

TOptional<float> FEpithColumnLayout::GetNameWidth(float RowWidth) const
{
	const TOptional<float> Width = NameFraction > 0.0f ? TOptional<float>(FMath::RoundToFloat(RowWidth * NameFraction)) : GetAutomaticNameWidth();

	if (!Width.IsSet())
	{
		return Width;
	}

	// The value column (at its narrowest), the reset column and the lines between them always fit
	const float MaxWidth = FMath::Max(0.0f, RowWidth - (LineWidth * 2.0f + MinValueWidth + ResetColumnWidth));

	return FMath::Clamp(Width.GetValue(), 0.0f, MaxWidth);
}

TOptional<float> FEpithColumnLayout::GetAutomaticNameWidth() const
{
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

	return CachedNameWidth > 0.0f ? TOptional<float>(CachedNameWidth) : TOptional<float>();
}

void FEpithColumnLayout::MeasureNow(const TSharedRef<SWidget>& NewContent)
{
	// One layout pass caches every new row's desired size. The column widths it produces are thrown away (some rows were
	// measured after the columns were sized); the real pass this frame works them out again from the complete measurements.
	NewContent->SlatePrepass();
	++Generation;
}

FEpithDrawContext FEpithDrawContext::WithColumns(const FEpithColumnSettings& Settings, EEpithGroupAxis ChildAxis) const
{
	FEpithDrawContext ChildContext = *this;
	ChildContext.ParentAxis = ChildAxis;
	ChildContext.DividerReach = FMargin(0.0f);
	ChildContext.RowEdges = FEpithRowEdges::Standalone();
	ChildContext.Columns = MakeShared<FEpithColumnLayout>(Settings.NameColumnFraction, &Settings);
	return ChildContext;
}

bool EpithDraw::IsFilledDivider(const TInstancedStruct<FEpithWindowElement>& Slot)
{
	const FEpithWindowElement_Divider* Divider = Slot.GetPtr<FEpithWindowElement_Divider>();
	return Divider && Divider->bFilled;
}
