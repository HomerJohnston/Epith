#pragma once

#include "CoreMinimal.h"

class IDetailTreeNode;
class SWidget;
struct FEpithPropertyDataContainer;
class FEpithColumnLayout;

/**
 * Draws property rows the way the Details panel does: edit-condition toggle, name, value, reset-to-default button,
 * and expandable struct/array children. Rows hide themselves when the details system hides them (e.g. EditConditionHides),
 * and are shown read-only when the property isn't editable on placed instances.
 */
namespace EpithPropertyRow
{
	/** @param Columns The group's shared column widths, so rows in the same group line up. Null = no alignment. */
	TSharedRef<SWidget> Make(TSharedRef<IDetailTreeNode> Node, TSharedRef<FEpithPropertyDataContainer> PropertyData, int32 Depth = 0, TSharedPtr<FEpithColumnLayout> Columns = nullptr);
}
