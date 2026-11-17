#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"

class SWidget;
class UEpithPanelDefinition;

/**
 * The menu of property paths on a target class, grouped by the class or struct that declares them.
 * Used by the property selector in the Details panel and by the property picker in the panel designer.
 */
namespace EpithPropertyMenu
{
	/** What the tree lines draw in one indent column of a row. */
	enum class ETreeLine : uint8
	{
		Empty,   // nothing
		Through, // │  an ancestor's later siblings continue below
		Tee,     // ├  this row, with more siblings below
		Elbow,   // └  this row, the last of its siblings
	};

	/** One pickable property path, as listed in the property menu and the designer's Properties pane. */
	struct FListRow
	{
		FName Path;

		/** Its own name; nested rows sit under the property they're in, so they don't repeat its name. */
		FText Label;

		/** 0 for a top-level property, 1 for its members, and so on. */
		int32 NestingLevel = 0;

		/** Tree lines down the row's left side, one per nesting level. */
		TArray<ETreeLine> TreeColumns;

		/** The property's tooltip, its path, and notes (read-only, inherited). Not the "already used" note, which changes as you edit. */
		FText ToolTip;

		/** The parent class the top-level property is declared on, or null if it's declared on the target class itself. */
		const UClass* InheritedFrom = nullptr;
	};

	/** The rows from one class: the target class first, then its parent, and so on. Rows are in tree order. */
	struct FListSection
	{
		FText Heading;
		TArray<FListRow> Rows;
	};

	struct FList
	{
		/** False if there's no instance of the target class in the level, so the list comes from the class defaults. */
		bool bFromInstance = false;

		TArray<FListSection> Sections;
	};

	/** Every pickable property path on TargetClass, by class and in tree order, with tree lines worked out. */
	FList GatherList(UClass* TargetClass);

	/** The tree lines for one row. */
	TSharedRef<SWidget> MakeTreeLines(const TArray<ETreeLine>& Columns);

	/** Explains a list that came from the class defaults (FList::bFromInstance is false). */
	FText GetFromDefaultsNote(const UClass* TargetClass);

	/** A row's tooltip with the "already shown in this panel" note added. */
	FText AddUsedNote(const FText& ToolTip);

	/** A row's text color: green if already used in the panel, dimmer if inherited. */
	FSlateColor GetRowColor(bool bUsed, bool bInherited);

	/** A row's text opacity: used and inherited rows are dimmed. */
	float GetRowOpacity(bool bUsed, bool bInherited);

	/** @param UsedPaths Paths already shown by Property elements in the panel; they're listed in green. */
	TSharedRef<SWidget> Make(UClass* TargetClass, TFunction<void(FName)> OnPicked, const TSet<FName>& UsedPaths = TSet<FName>());

	/** Every property path used by a Property element anywhere in the definition's layout. */
	TSet<FName> GatherUsedPaths(const UEpithPanelDefinition* Definition);

	/** A wrapped text message, for menus that have nothing to offer. */
	TSharedRef<SWidget> MakeMessage(const FText& Message);
}
