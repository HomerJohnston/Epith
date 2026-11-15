// Copyright Ghost Pepper Games, Inc. All Rights Reserved.
// This work is MIT-licensed. Feel free to use it however you wish, within the confines of the MIT license.

#pragma once

#include "Styling/SlateStyle.h"
#include "Delegates/IDelegateInstance.h"
#include "Textures/SlateIcon.h"
#include "UObject/StrongObjectPtr.h"

#define LOCTEXT_NAMESPACE "EpithEditor"

// ==============================================

struct FEpithBrushes
{
	// Borders: thin (2px) and thick (3px), as a box or U-shaped (open at the top, for under a foldout bar or tab bar)
	FName Border_2px_Deburred;
	FName Border_3px_Deburred;
	FName Border_2px_Deburred_U;
	FName Border_3px_Deburred_U;
	FName Border_TabShape;
	FName Background_ViewportMasterPanelHeader;

	FName Background_WhiteSquare;
};

struct FEpithStyles
{
	FName CheckBoxStyle_Tab;
	FName SegmentedControlStyle_Tab;
};

struct FEpithFonts
{
	/** The title of the whole panel, in its header bar. */
	FSlateFontInfo Font_ViewportPopup_PanelTitle;

	/** Section titles inside the panel (one per object). */
	FSlateFontInfo Font_ViewportPopup_Title;
};

extern FEpithBrushes EpithBrushes;
extern FEpithStyles EpithStyles;
extern FEpithFonts EpithFonts;

class FEpithStyle
{
public:

public:
    FEpithStyle();

    virtual ~FEpithStyle();

public:
    static ISlateStyle& Get();

    static const FSlateBrush* GetImageBrush(FName BrushName);

    /** The border brush for a Border setting (0 = none, 1 = thin, 2 = thick). bOpenTop gives the U-shaped version. */
    static const FSlateBrush* GetBorderBrush(uint8 Border, bool bOpenTop);

    /** Padding that keeps content inside that border (none at the top when bOpenTop). */
    static FMargin GetBorderPadding(uint8 Border, bool bOpenTop);

    static FName GetStyleSetName();

    /** Creates and registers the style set. Called once from FEpithModule::StartupModule. Safe to call more than once. */
    static void Initialize();

    /** Unregisters and destroys the style set. Called from FEpithModule::ShutdownModule. */
    static void Shutdown();

protected:
    static TSharedRef< class FSlateStyleSet > Create();

    static void SetupStyles();

    static void OnPatchComplete();

    static const ISlateStyle* GetParentStyle();

    static TArray<TStrongObjectPtr<UTexture2D>> Textures;

    static FDelegateHandle OnPatchCompleteHandle;

    static TSharedPtr<FSlateStyleSet> StyleInstance;

};

#undef LOCTEXT_NAMESPACE