#pragma once

#include "CoreMinimal.h"
#include "Epith/Widgets/EpithDrawContext.h"
#include "Epith/Widgets/EpithWindowElement_Property.h"

class IDetailTreeNode;
class SWidget;
struct FEpithPropertyDataContainer;
class FEpithColumnLayout;

/**
 * Draws property rows the way the Details panel does: edit-condition toggle, name, value, reset-to-default button,
 * and expandable struct/array children. Rows hide themselves when the details system hides them (e.g. EditConditionHides),
 * and are shown read-only when the property isn't editable on placed instances.
 */
/** How a row shows its children (see EEpithPropertyChildDisplay), and whether its children's rows do the same. */
struct FEpithRowChildDisplay
{
	EEpithPropertyChildDisplay Mode = EEpithPropertyChildDisplay::Foldout;

	/** Child rows show their own children the same way; otherwise they're ordinary foldouts. */
	bool bRecursive = false;

	/** What this row's child rows get. */
	FEpithRowChildDisplay ForChildren() const
	{
		return bRecursive ? *this : FEpithRowChildDisplay();
	}
};

namespace EpithPropertyRow
{
	/**
	 * @param Columns The group's shared column widths, so rows in the same group line up. Null = no alignment.
	 * @param Edges   Grid lines along the row's top and bottom. Child rows (struct members, array elements) keep the bottom line.
	 * @param ChildDisplay How the row shows its children: a foldout, always shown, or only the children.
	 */
	TSharedRef<SWidget> Make(TSharedRef<IDetailTreeNode> Node, TSharedRef<FEpithPropertyDataContainer> PropertyData, int32 Depth = 0, TSharedPtr<FEpithColumnLayout> Columns = nullptr, const FEpithRowEdges& Edges = FEpithRowEdges(), const FEpithRowChildDisplay& ChildDisplay = FEpithRowChildDisplay());
}
