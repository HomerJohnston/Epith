#pragma once

#include "CoreMinimal.h"

class SWidget;
class UEpithPanelDefinition;

/**
 * The menu of property paths on a target class, grouped by the class or struct that declares them.
 * Used by the property selector in the Details panel and by the property picker in the panel designer.
 */
namespace EpithPropertyMenu
{
	/** @param UsedPaths Paths already shown by Property elements in the panel; they're listed in green. */
	TSharedRef<SWidget> Make(UClass* TargetClass, TFunction<void(FName)> OnPicked, const TSet<FName>& UsedPaths = TSet<FName>());

	/** Every property path used by a Property element anywhere in the definition's layout. */
	TSet<FName> GatherUsedPaths(const UEpithPanelDefinition* Definition);

	/** A wrapped text message, for menus that have nothing to offer. */
	TSharedRef<SWidget> MakeMessage(const FText& Message);
}
