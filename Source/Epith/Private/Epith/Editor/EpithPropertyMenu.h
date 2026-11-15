#pragma once

#include "CoreMinimal.h"

class SWidget;

/**
 * The menu of property paths on a target class, grouped by the class or struct that declares them.
 * Used by the property selector in the Details panel and by the property picker in the panel designer.
 */
namespace EpithPropertyMenu
{
	TSharedRef<SWidget> Make(UClass* TargetClass, TFunction<void(FName)> OnPicked);
	
	/** A wrapped text message, for menus that have nothing to offer. */
	TSharedRef<SWidget> MakeMessage(const FText& Message);
}
